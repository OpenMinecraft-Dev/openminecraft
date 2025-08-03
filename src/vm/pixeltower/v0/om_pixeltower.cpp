#include "openminecraft/vm/pixeltower/v0/om_pixeltower.hpp"
#include "openminecraft/log/om_log_common.hpp"
#include "openminecraft/mem/om_mem_allocator.hpp"
#include "openminecraft/util/om_util_result.hpp"
#include "openminecraft/vm/classfile/om_class_file.hpp"
#include "openminecraft/vm/pixeltower/v0/om_pixeltower_heap.hpp"
#include "openminecraft/vm/pixeltower/v0/om_pixeltower_interpreter.hpp"
#include "openminecraft/vm/pixeltower/v0/om_pixeltower_klass.hpp"
#include "openminecraft/vm/pixeltower/v0/om_pixeltower_klassloader.hpp"
#include "openminecraft/vm/pixeltower/v0/om_pixeltower_method.hpp"
#include "openminecraft/vm/pixeltower/v0/om_pixeltower_threads.hpp"
#include <filesystem>
#include <fstream>
#include <iostream>
#include <memory>

namespace openminecraft::vm::pixeltower::v0
{
std::any println_impl(std::any *)
{
    std::cout << "printing!" << std::endl;
    *(int *)nullptr = 0;
    return nullptr;
}

void OMPixelTower::handleCrash()
{
    bool inNative = currentThread.currentFrame->method->accessFlags & JVM_Acc_Native;
    logger.debug("in native: {}", inNative);

    auto fr = currentThread.currentFrame;
    while (fr)
    {
	logger.info("{}.{}{} returns to {}", fr->method->klass->name, fr->method->name, fr->method->desc, fmt::ptr(fr->returnAddr));
	if (!metaspace->inside(fr->returnAddr)) {
	    logger.info("entering native frames!");
	}
	fr = fr->prev;
    }
}
OMPixelTower::OMPixelTower() : logger("OMPixelTower", this)
{
    heap = new OMPixelTowerHeap(1ul * 1024 * 1024, 1ul * 1024 * 1024 * 1024);
    metaspace = new OMPixelTowerHeap(1ul * 1024 * 1024, 8ul * 1024 * 1024);
    interpreter = new OMInterpreter(heap, this);
    loader = new OMKlassLoader(heap, metaspace, interpreter);

    loader->nativeMethods["vmstd/internal/SystemPrintStream.println(J)V"] = println_impl;
}
OMPixelTower::~OMPixelTower()
{
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
}

void OMPixelTower::destroyCurrentThread()
{
    if (currentThread.stackEnd)
    {
        mem::allocator::tracedFreeVMData(currentThread.stackEnd);
    }
}
} // namespace openminecraft::vm::pixeltower::v0
