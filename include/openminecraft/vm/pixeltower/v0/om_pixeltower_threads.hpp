#ifndef OM_PIXELTOWER_THREADS_HPP
#define OM_PIXELTOWER_THREADS_HPP

#include "openminecraft/vm/pixeltower/v0/om_pixeltower_base.hpp"
#include "openminecraft/vm/pixeltower/v0/om_pixeltower_frame.hpp"
#include <string>
#include <thread>
#include <type_traits>
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

inline void stackPush()
{
    currentThread.stackPointer = (uint8_t *)currentThread.stackPointer - sizeof(void *);
}

inline void stackPop()
{
    currentThread.stackPointer = (uint8_t *)currentThread.stackPointer + sizeof(void *);
}

inline void stackPopW()
{
    stackPop();
    stackPop();
}

template <typename T> inline T stackTopAccess()
{
    static_assert(std::is_same_v<T, jint> || std::is_same_v<T, void *> || std::is_same_v<T, jfloat>,
                  "unsatisfied type!");
    return (*(T *)((void **)currentThread.stackPointer + 1));
}

template <typename T> inline void stackPushAccess(T data)
{
    static_assert(std::is_same_v<T, jint> || std::is_same_v<T, void *> || std::is_same_v<T, jfloat>,
                  "unsatisfied type!");
    *static_cast<void **>(currentThread.stackPointer) = nullptr; // clears the whole slot
    *static_cast<T *>(currentThread.stackPointer) = data;
    stackPush();
}

template <typename T> inline T stackTopAccessW()
{
    static_assert(std::is_same_v<T, jdouble> || std::is_same_v<T, jlong>, "unsatisfied type!");
    if (sizeof(void *) == 8)
    {
        return (*(T *)((void **)currentThread.stackPointer + 2)); // padding
    }
    else
    {
        auto highbits = (int64_t)stackTopAccess<jint>();
        auto lowbits = (*(jint *)((void **)currentThread.stackPointer + 2));

        int64_t temp = (highbits << 32) + lowbits;

        return *(T *)&temp;
    }
}

template <typename T> inline void stackPushAccessW(T data)
{
    static_assert(std::is_same_v<T, jdouble> || std::is_same_v<T, jlong>, "unsatisfied type!");
    // 64-bit impl
    if (sizeof(void *) == 8)
    {
        *(T *)currentThread.stackPointer = data;
        stackPush();
        stackPushAccess<void *>(nullptr);
    }
    else
    {
        auto raw = (jint *)&data;
        stackPushAccess<jint>(data);       // low bits
        stackPushAccess<jint>(data >> 32); // high bits
    }
}

template <typename T> inline T *localAccess(int idx, OMFrame *f = currentThread.currentFrame)
{
    static_assert(std::is_same_v<T, jint> || std::is_same_v<T, void *> || std::is_same_v<T, jfloat>,
                  "unsatisfied type!");
    return (T *)(((uint8_t *)f) - sizeof(void *) * (f->method->maxLocals - idx));
}

template <typename T> inline T localAccessValue(int idx, OMFrame *f = currentThread.currentFrame)
{
    static_assert(std::is_same_v<T, jint> || std::is_same_v<T, void *> || std::is_same_v<T, jfloat>,
                  "unsatisfied type!");
    return *(T *)(((uint8_t *)f) - sizeof(void *) * (f->method->maxLocals - idx));
}

template <typename T> inline T localAccessValueW(int idx, OMFrame *f = currentThread.currentFrame)
{
    static_assert(std::is_same_v<T, jlong> || std::is_same_v<T, jdouble>, "unsatisfied type!");
    if (sizeof(void *) == 8)
    {
        return *(T *)(((uint8_t *)f) - sizeof(void *) * (f->method->maxLocals - idx));
    }
    else
    {
        auto lowbits = localAccessValue<jint>(idx, f);
        auto highbits = (int64_t)localAccessValue<jint>(idx + 1, f);

        int64_t temp = (highbits << 32) + lowbits;

        return *(T *)temp;
    }
}

} // namespace openminecraft::vm::pixeltower::v0

#endif
