#include "openminecraft/vm/pixeltower/v0/om_pixeltower.hpp"
#include "openminecraft/log/om_log_common.hpp"
#include "openminecraft/mem/om_mem_allocator.hpp"
#include "openminecraft/util/om_util_result.hpp"
#include "openminecraft/vm/classfile/om_class_file.hpp"
#include "openminecraft/vm/pixeltower/v0/om_pixeltower_base.hpp"
#include "openminecraft/vm/pixeltower/v0/om_pixeltower_frame.hpp"
#include "openminecraft/vm/pixeltower/v0/om_pixeltower_heap.hpp"
#include "openminecraft/vm/pixeltower/v0/om_pixeltower_interpreter.hpp"
#include "openminecraft/vm/pixeltower/v0/om_pixeltower_method.hpp"
#include "openminecraft/vm/pixeltower/v0/om_pixeltower_threads.hpp"
#include <filesystem>
#include <fstream>
#include <memory>

namespace openminecraft::vm::pixeltower::v0
{
OMPixelTower::OMPixelTower() : logger("OMPixelTower", this)
{
    heap = new OMPixelTowerHeap(1ul * 1024 * 1024, 1ul * 1024 * 1024 * 1024);
    metaspace = new OMPixelTowerHeap(1ul * 1024 * 1024, 8ul * 1024 * 1024);
    loader = new OMKlassLoader(heap, metaspace);
    interpreter = new OMInterpreter(heap);
}
OMPixelTower::~OMPixelTower()
{
    delete interpreter;
    delete loader;
    delete heap;
    delete metaspace;
}

void OMPixelTower::stackTest(OMMethod *method)
{
    auto frame = (OMFrame *)(currentThread.currentFrame == nullptr ? currentThread.stack
                                                                   : currentThread.currentFrame->stackPointer);
    frame->stackPointer = (jbyte *)currentThread.stack + sizeof(OMFrame);
    frame->returnAddr = currentThread.pc;
    frame->method = method;

    frame->stackPointer = (uint8_t *)frame->stackPointer + method->maxLocals * sizeof(void *);

    currentThread.currentFrame = frame;
    currentThread.pc = method->code;

    logger.debug("method {} code {}", (void *)method, (void *)method->code);
    logger.debug("stack: {}", currentThread.stack);

    interpreter->execute();
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
    if (currentThread.stack)
    {
        return;
    }

    currentThread.stack = mem::allocator::tracedCallocVMData(1, tlsSize);
}

void OMPixelTower::destroyCurrentThread()
{
    if (currentThread.stack)
    {
        mem::allocator::tracedFreeVMData(currentThread.stack);
    }
}
} // namespace openminecraft::vm::pixeltower::v0