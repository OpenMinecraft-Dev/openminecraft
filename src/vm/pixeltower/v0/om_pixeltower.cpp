#include "openminecraft/vm/pixeltower/v0/om_pixeltower.hpp"
#include "openminecraft/log/om_log_common.hpp"
#include "openminecraft/mem/om_mem_allocator.hpp"
#include "openminecraft/util/om_util_result.hpp"
#include "openminecraft/vm/classfile/om_class_file.hpp"
#include "openminecraft/vm/impl/om_impl_printstream.hpp"
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

namespace openminecraft::vm::pixeltower::v0
{
void OMPixelTower::handleCrash(int code, int pid, std::vector<v1::tracing::OMTracingFrame> &frames)
{
    logger.error("{} crashed with code {}", pid, code);

    auto nativeFrameInPt = [](v1::tracing::OMTracingFrame &f) {
        return f.name.rfind("openminecraft::vm::pixeltower", 0) == 0;
    };

    bool findedFrame = false;
    for (auto fr : frames)
    {
        if (fr.name.rfind("openminecraft::vm::pixeltower", 0) == 0)
        {
            findedFrame = true;
            break;
        }
    }

    bool inNative = currentThread.currentFrame->method->accessFlags & JVM_Acc_Native;
    logger.debug("in native: {}", inNative);

    if (findedFrame)
    {
        logger.error("pixeltower frames found!");
        auto fr = currentThread.currentFrame;
        void *tracingPC = currentThread.pc;
        auto nfitt = frames.begin();

        while (nfitt != frames.end())
        {
            if (nativeFrameInPt(*nfitt))
            {
                while (nativeFrameInPt(*nfitt))
                {
                    ++nfitt;
                }

                logger.error("J {}.{}{} + {}", fr->method->klass->name, fr->method->name, fr->method->desc,
                             reinterpret_cast<size_t>(tracingPC) - reinterpret_cast<size_t>(fr->method->code));
                tracingPC = fr->returnAddr;
                fr = fr->prev;
                while (fr != nullptr && (fr->method->accessFlags & JVM_Acc_Native) == 0)
                {
                    logger.error("J {}.{}{} + {}", fr->method->klass->name, fr->method->name, fr->method->desc,
                                 reinterpret_cast<size_t>(tracingPC) - reinterpret_cast<size_t>(fr->method->code));
                    tracingPC = fr->returnAddr;
                    fr = fr->prev;
                }

                continue;
            }

            logger.error("C {} @ {}", nfitt->location, nfitt->name == "" ? "???" : nfitt->name);
            ++nfitt;
        }
    }
    else
    {
        logger.error("debug info stripped or corrupted, only shows pixeltower vm frames");
        logger.error("C {} @ {}", frames[0].location, frames[0].name);
        logger.error("(unknown call stack)");
        auto fr = currentThread.currentFrame;
        while (fr)
        {
            logger.error("J {}.{}{}", fr->method->klass->name, fr->method->name, fr->method->desc);
            fr = fr->prev;
        }
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
    interpreter->call(method, (uint8_t *)nullFunction);
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
    init(target);
}

void OMPixelTower::load(std::string path)
{
    std::vector<std::shared_ptr<std::istream>> target;
    target.push_back(std::make_shared<std::ifstream>(path, std::ios::binary));
    init(target);
}

void OMPixelTower::init(std::vector<std::shared_ptr<std::istream>> &streams)
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
    loader->loadClass("[B"); // class for byte[]
    auto barr = loader->fetchClass("[B");
    auto att = barr->allocateArray(str.length());
    att->mark |= mconst;
    std::memcpy(att->array<jbyte>(), str.c_str(), att->length);
    loader->loadClass("java/lang/String");
    auto stt = loader->fetchClass("java/lang/String");
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
    return tgt;
}
} // namespace openminecraft::vm::pixeltower::v0
