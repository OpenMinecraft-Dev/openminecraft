#include "openminecraft/vm/pixeltower/v0/om_pixeltower.hpp"
#include "openminecraft/binary/om_bin_hash.hpp"
#include "openminecraft/log/om_log_common.hpp"
#include "openminecraft/mem/om_mem_allocator.hpp"
#include "openminecraft/util/om_util_result.hpp"
#include "openminecraft/vm/bytecode/om_bytecode_descriptor.hpp"
#include "openminecraft/vm/classfile/om_class_file.hpp"
#include "openminecraft/vm/impl/om_impl_object.hpp"
#include "openminecraft/vm/impl/om_impl_printstream.hpp"
#include "openminecraft/vm/impl/om_impl_throwable.hpp"
#include "openminecraft/vm/pixeltower/v0/om_pixeltower_heap.hpp"
#include "openminecraft/vm/pixeltower/v0/om_pixeltower_interpreter.hpp"
#include "openminecraft/vm/pixeltower/v0/om_pixeltower_klass.hpp"
#include "openminecraft/vm/pixeltower/v0/om_pixeltower_klassloader.hpp"
#include "openminecraft/vm/pixeltower/v0/om_pixeltower_method.hpp"
#include "openminecraft/vm/pixeltower/v0/om_pixeltower_oop.hpp"
#include "openminecraft/vm/pixeltower/v0/om_pixeltower_threads.hpp"
#include "openminecraft/vm/pixeltower/v1/om_pixeltower_tracing.hpp"
#include "openminecraft/vm/pixeltower/v2/om_pixeltower_gc_serial.hpp"
#include <cstdint>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <memory>
#include <thread>
#include <vector>

namespace openminecraft::vm::pixeltower::v0
{
void OMPixelTower::handleCrash(int code, int pid, std::vector<v1::tracing::OMTracingFrame> &frames)
{
    logger.error("{} crashed with code {}", pid, code);

    logger.error("traceback:");
    auto d = v1::tracing::fetchFrames(frames);
    while (d)
    {
        if (d->type == v1::tracing::JavaFrame || d->type == v1::tracing::JavaJITFrame)
        {
            logger.error("J {}.{}{} + {}", d->jvm.method->klass->name, d->jvm.method->name, d->jvm.method->desc,
                         d->jvm.offset);
        }
        else
        {
            logger.error("C {} @ {}", d->native.name, d->target);
        }
        d = d->next;
    }

    logger.error("register dumps: ");
    for (auto tar : v1::tracing::registers)
    {
        logger.error("{}: {}", tar.first, tar.second);
    }
}
OMPixelTower::OMPixelTower() : logger("OMPixelTower", this)
{
    heap = new OMPixelTowerHeap(1ul * 1024 * 1024, 1ul * 1024 * 1024 * 1024);
    metaspace = new OMPixelTowerHeap(1ul * 1024 * 1024, 8ul * 1024 * 1024);
    interpreter = new OMInterpreter(heap, this);
    loader = new OMKlassLoader(heap, metaspace, interpreter);
    gc = new v2::OMGarbageCollectorSerial(heap, this);

    loader->nativeMethods["vmstd/internal/SystemPrintStream.println(J)V"] =
        impl::vmstd_internal_SystemPrintStream_println;
    loader->nativeMethods["vmstd/internal/SystemPrintStream.println(F)V"] =
        impl::vmstd_internal_SystemPrintStream_println;
    loader->nativeMethods["vmstd/internal/SystemPrintStream.println(Ljava/lang/String;)V"] =
        impl::vmstd_internal_SystemPrintStream_println;
    loader->nativeMethods["vmstd/internal/SystemPrintStream.println(Ljava/lang/Object;)V"] =
        impl::vmstd_internal_SystemPrintStream_println;
    loader->nativeMethods["vmstd/internal/SystemPrintStream.println(D)V"] =
        impl::vmstd_internal_SystemPrintStream_println;
    loader->nativeMethods["vmstd/internal/SystemPrintStream.println(I)V"] =
        impl::vmstd_internal_SystemPrintStream_println;
    loader->nativeMethods["vmstd/internal/SystemPrintStream.println(Z)V"] =
        impl::vmstd_internal_SystemPrintStream_println;
    loader->nativeMethods["java/lang/Object.hashCode()I"] = impl::java_lang_Object_hashCode;
    loader->nativeMethods["java/lang/Throwable.fillInStackTrace()V"] = impl::java_lang_Throwable_fillInStackTrace;
}
OMPixelTower::~OMPixelTower()
{
    delete gc;
    delete interpreter;
    delete loader;
    delete heap;
    delete metaspace;
}

void OMPixelTower::boot(OMMethod *method)
{
    interpreter->call(method, static_cast<uint8_t *>(nullFunction));
}

void OMPixelTower::init(std::string basePath)
{
    std::vector<std::shared_ptr<std::istream>> target;
    std::filesystem::recursive_directory_iterator itt(basePath);
    for (std::filesystem::recursive_directory_iterator end; end != itt; ++itt)
    {
        if (itt->is_regular_file() && itt->path().extension() == ".class")
        {
            target.push_back(std::make_shared<std::ifstream>(itt->path(), std::ios::binary));
            logger.debug("{} found", itt->path().string());
        }
    }
    initStreams(target);

    loader->initBase();
}

void OMPixelTower::load(std::string path)
{
    std::vector<std::shared_ptr<std::istream>> target;
    target.push_back(std::make_shared<std::ifstream>(path, std::ios::binary));
    initStreams(target);
}

void OMPixelTower::initStreams(std::vector<std::shared_ptr<std::istream>> &streams)
{
    for (auto &s : streams)
    {
        classfile::OMClassFileParser parser(&*s);
        auto target = parser.parse();

        if (target.type == util::Err)
        {
            throw target.unwrap_err();
        }

        loader->stagClass(target.unwrap());
    }
}

void OMPixelTower::initCurrentThread(uint64_t tlsSize)
{
    if (currentThread.stackEnd)
    {
        return;
    }

    currentThread.stackEnd = mem::allocator::tracedCallocVMData(1, tlsSize);
    currentThread.stack = (uint8_t *)currentThread.stackEnd + tlsSize;
    currentThread.stackPointer = currentThread.stack;

    threadMap[std::this_thread::get_id()] = &currentThread;
}

void OMPixelTower::destroyCurrentThread()
{
    if (currentThread.stackEnd)
    {
        mem::allocator::tracedFreeVMData(currentThread.stackEnd);
    }
}

void *OMPixelTower::createString(std::string str)
{
    auto hsh = binary::hash::hash_compile_time(str.c_str());
    if (pooledStrings.count(hsh))
    {
        return pooledStrings[hsh];
    }

    bytecode::descriptor::OMTypeDesc barrdesc = {bytecode::descriptor::Array, "", 1, bytecode::descriptor::Byte};
    loader->loadClass(barrdesc); // class for byte[]
    auto barr = loader->fetchClass(barrdesc);
    auto att = barr->allocateArray(str.length());
    // geopelia: this object will never be collected!
    att->mark |= mconst;
    std::memcpy(att->array<jbyte>(), str.c_str(), att->length);
    bytecode::descriptor::OMTypeDesc strdesc = {bytecode::descriptor::Reference, "java/lang/String"};
    loader->loadClass(strdesc);
    auto stt = loader->fetchClass(strdesc);
    auto tgt = stt->allocateInstance();
    tgt->mark |= mconst;
    if (heap->ptrCompEnabled())
    {
        *reinterpret_cast<uint32_t *>(tgt->data) = heap->compressPtr(att);
    }
    else
    {
        *reinterpret_cast<void **>(tgt->data) = att;
    }
    pooledStrings[hsh] = tgt;
    return tgt;
}
} // namespace openminecraft::vm::pixeltower::v0
