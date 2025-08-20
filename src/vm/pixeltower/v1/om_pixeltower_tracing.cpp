#include "openminecraft/vm/pixeltower/v1/om_pixeltower_tracing.hpp"
#include "boost/stacktrace/stacktrace.hpp"
#include "openminecraft/mem/om_mem_allocator.hpp"
#include "openminecraft/vm/pixeltower/v0/om_pixeltower_threads.hpp"
#include <vector>

namespace openminecraft::vm::pixeltower::v1::tracing
{
OMParsedFrame *fetchFrames()
{
    std::vector<OMTracingFrame> frames;
    boost::stacktrace::stacktrace st;
    for (auto &frame : st)
    {
        frames.push_back(OMTracingFrame{(void *)frame.address(), frame.name()});
    }
    return fetchFrames(frames);
}

OMParsedFrame *fetchFrames(std::vector<OMTracingFrame> &frames)
{
    std::vector<OMParsedFrame *> target;

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

    if (findedFrame)
    {
        auto fr = v0::currentThread.currentFrame;
        void *tracingPC = v0::currentThread.pc;
        auto nfitt = frames.begin();

        while (nfitt != frames.end())
        {
            if (nativeFrameInPt(*nfitt))
            {
                while (nativeFrameInPt(*nfitt))
                {
                    ++nfitt;
                }

                while (true)
                {
                    auto root = (OMParsedFrame *)mem::allocator::tracedCallocVMData(1, sizeof(OMParsedFrame));
                    root->target = tracingPC;
                    root->type = JavaFrame;
                    root->jvm.method = fr->method;
                    root->jvm.offset = reinterpret_cast<size_t>(tracingPC) - reinterpret_cast<size_t>(fr->method->code);
                    target.push_back(root);

                    tracingPC = fr->returnAddr;
                    fr = fr->prev;

                    if (fr == nullptr || (fr->method->accessFlags & JVM_Acc_Native))
                    {
                        break;
                    }
                }

                continue;
            }

            auto root = (OMParsedFrame *)mem::allocator::tracedCallocVMData(1, sizeof(OMParsedFrame));
            root->target = nfitt->location;
            root->type = NativeFrame;
            auto tgt = nfitt->name.size() == 0 ? "???" : nfitt->name;
            tgt.copy(root->native.name, 255);
            root->native.name[255] = 0;
            target.push_back(root);

            ++nfitt;
        }
    }
    else
    {
        auto root = (OMParsedFrame *)mem::allocator::tracedCallocVMData(1, sizeof(OMParsedFrame));
        root->target = frames[0].location;
        root->type = NativeFrame;
        auto tgt = frames[0].name.size() == 0 ? "???" : frames[0].name;
        tgt.copy(root->native.name, 255);
        root->native.name[255] = 0;
        target.push_back(root);

        auto fr = v0::currentThread.currentFrame;
        void *tracingPC = v0::currentThread.pc;
        while (fr)
        {
            auto root = (OMParsedFrame *)mem::allocator::tracedCallocVMData(1, sizeof(OMParsedFrame));
            root->target = tracingPC;
            root->type = JavaFrame;
            root->jvm.method = fr->method;
            root->jvm.offset = reinterpret_cast<size_t>(tracingPC) - reinterpret_cast<size_t>(fr->method->code);
            target.push_back(root);

            tracingPC = fr->returnAddr;
            fr = fr->prev;
        }
    }

    // gino: build the contents into a linked list
    OMParsedFrame *current = nullptr;
    for (auto it = target.rbegin(); it != target.rend(); ++it)
    {
        auto ptt = *it;
        if (current)
        {
            ptt->next = current;
        }
        current = ptt;
    }

    return current;
}
} // namespace openminecraft::vm::pixeltower::v1::tracing