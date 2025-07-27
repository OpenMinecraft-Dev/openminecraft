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
    void *stackEnd;
};

#define stackBump                                                                                                      \
    currentThread.currentFrame->stackPointer = (uint8_t *)currentThread.currentFrame->stackPointer - sizeof(void *);
#define stackPushPointer(p)                                                                                            \
    *(void **)currentThread.currentFrame->stackPointer = p;                                                            \
    stackBump;
#define stackPushInt(i)                                                                                                \
    *(jint **)currentThread.currentFrame->stackPointer = i;                                                            \
    stackBump;
#define stackTopPointer (*((void **)currentThread.currentFrame->stackPointer + 1))
#define stackTopInt (*(jint *)((void **)currentThread.currentFrame->stackPointer + 1))

extern thread_local OMPixelTowerThread currentThread;
} // namespace openminecraft::vm::pixeltower::v0

#endif