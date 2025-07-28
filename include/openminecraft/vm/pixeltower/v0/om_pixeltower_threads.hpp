#ifndef OM_PIXELTOWER_THREADS_HPP
#define OM_PIXELTOWER_THREADS_HPP

#include "openminecraft/vm/pixeltower/v0/om_pixeltower_frame.hpp"
#include <string>
#include <thread>
namespace openminecraft::vm::pixeltower::v0
{
class OMPixelTowerThread
{
  public:
    OMPixelTowerThread() = default;
    ~OMPixelTowerThread() = default;

    std::thread::id id;
    std::string name;
    uint8_t *pc;
    OMFrame *currentFrame;
    void *stack;
    void *stackPointer;
    void *stackEnd;
};

extern thread_local OMPixelTowerThread currentThread;

#define stackPush currentThread.stackPointer = (uint8_t *)currentThread.stackPointer - sizeof(void *);
#define stackPop currentThread.stackPointer = (uint8_t *)currentThread.stackPointer + sizeof(void *);

template <typename T> T stackTopAccess()
{
    return (*(T *)((void **)currentThread.stackPointer + 1));
}

template <typename T> void stackPushAccess(T data)
{
    *(void **)currentThread.stackPointer = nullptr; // clears the whole slot
    *(T *)currentThread.stackPointer = data;
    stackPush;
}

#define localAccess(idx) localAccessForeign(currentThread.currentFrame, idx)
#define localAccessForeign(f, idx) (((uint8_t *)f) - sizeof(void *) * (f->method->maxLocals - (idx)))

} // namespace openminecraft::vm::pixeltower::v0

#endif
