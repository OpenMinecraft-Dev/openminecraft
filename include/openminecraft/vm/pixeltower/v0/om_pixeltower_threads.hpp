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

#define stackPush currentThread.stackPointer = (uint8_t *)currentThread.stackPointer - sizeof(void *);
#define stackPop currentThread.stackPointer = (uint8_t *)currentThread.stackPointer + sizeof(void *);

#define stackPushPointer(p)                                                                                            \
    *(void **)currentThread.stackPointer = p;                                                                          \
    stackPush;
#define stackPushInt(i)                                                                                                \
    *(void **)currentThread.stackPointer = nullptr;                                                                    \
    *(jint *)currentThread.stackPointer = i;                                                                           \
    stackPush;
#define stackTopPointer (*((void **)currentThread.stackPointer + 1))
#define stackTopInt (*(jint *)((void **)currentThread.stackPointer + 1))

#define localAccess(idx) (((uint8_t *)currentThread.currentFrame) - sizeof(void *) * (idx + 1))
#define localAccessForeign(f, idx) (((uint8_t *)f) - sizeof(void *) * (idx + 1))

extern thread_local OMPixelTowerThread currentThread;
} // namespace openminecraft::vm::pixeltower::v0

#endif
