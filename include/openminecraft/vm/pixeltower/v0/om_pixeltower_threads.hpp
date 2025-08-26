#ifndef OM_PIXELTOWER_THREADS_HPP
#define OM_PIXELTOWER_THREADS_HPP

#include "openminecraft/vm/bytecode/om_bytecode_descriptor.hpp"
#include "openminecraft/vm/pixeltower/v0/om_pixeltower_base.hpp"
#include "openminecraft/vm/pixeltower/v0/om_pixeltower_field.hpp"
#include "openminecraft/vm/pixeltower/v0/om_pixeltower_frame.hpp"
#include "openminecraft/vm/pixeltower/v0/om_pixeltower_method.hpp"
#include "openminecraft/vm/pixeltower/v0/om_pixeltower_oop.hpp"
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
    uint8_t *pc = nullptr;
    OMFrame *currentFrame = nullptr;
    void *stack = nullptr;
    void *stackPointer = nullptr;
    void *stackEnd = nullptr;
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

template <typename T> inline T stackTopAccess(bool needPop = false)
{
    static_assert(std::is_same_v<T, jint> || std::is_pointer_v<T> || std::is_same_v<T, jfloat>, "unsatisfied type!");
    T value = (*(T *)((void **)currentThread.stackPointer + 1));
    if (needPop)
    {
        stackPop();
    }
    return value;
}

template <typename T> inline void stackPushAccess(T data)
{
    static_assert(std::is_same_v<T, jint> || std::is_pointer_v<T> || std::is_same_v<T, jfloat>, "unsatisfied type!");
    *static_cast<void **>(currentThread.stackPointer) = nullptr; // clears the whole slot
    *static_cast<T *>(currentThread.stackPointer) = data;
    stackPush();
}

template <typename T> inline T stackTopAccessW(bool needPop = false)
{
    static_assert(std::is_same_v<T, jdouble> || std::is_same_v<T, jlong>, "unsatisfied type!");
    T value;
    if (sizeof(void *) == 8)
    {
        value = (*(T *)((void **)currentThread.stackPointer + 2)); // padding
    }
    else
    {
        auto highbits = (int64_t)stackTopAccess<jint>();
        auto lowbits = (*(jint *)((void **)currentThread.stackPointer + 2));

        int64_t temp = (highbits << 32) + lowbits;

        value = *(T *)&temp;
    }
    if (needPop)
    {
        stackPopW();
    }
    return value;
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
        auto raw = *(int64_t *)&data;

        stackPushAccess<jint>(raw & 0xffffffff); // low bits
        stackPushAccess<jint>(raw >> 32);        // high bits
    }
}

inline void *stackBottom(OMFrame *f = currentThread.currentFrame)
{
    return ((uint8_t *)f) - sizeof(void *) * f->method->maxLocals - sizeof(void *);
}

template <typename T> inline void localAccessMod(int idx, T value, OMFrame *f = currentThread.currentFrame)
{
    static_assert(std::is_same_v<T, jint> || std::is_pointer_v<T> || std::is_same_v<T, jfloat>, "unsatisfied type!");
    *(T *)(((uint8_t *)f) - sizeof(void *) * (f->method->maxLocals - idx)) = value;
}

template <typename T> inline void localAccessModW(int idx, T value, OMFrame *f = currentThread.currentFrame)
{
    static_assert(std::is_same_v<T, jlong> || std::is_same_v<T, jdouble>, "unsatisfied type!");
    if (sizeof(void *) == 8)
    {
        *(T *)(((uint8_t *)f) - sizeof(void *) * (f->method->maxLocals - idx)) = value;
    }
    else
    {
        int64_t temp = *reinterpret_cast<int64_t *>(&value);

        localAccessMod<jint>(idx, temp & 0xffffffff, f);
        localAccessMod<jint>(idx + 1, temp >> 32, f);
    }
}

template <typename T> inline T localAccessValue(int idx, OMFrame *f = currentThread.currentFrame)
{
    static_assert(std::is_same_v<T, jint> || std::is_pointer_v<T> || std::is_same_v<T, jfloat>, "unsatisfied type!");
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

        return *(T *)&temp;
    }
}

template <typename Ttarget, typename Tvalue> void accessFieldI(OMField *field)
{
    auto data = stackTopAccess<Tvalue>(true);
    auto obj = static_cast<OMOOPDesc *>(stackTopAccess<void *>(true));
    *reinterpret_cast<Ttarget *>(&obj->data[field->offset]) = data;
}

template <typename Ttarget, typename Tvalue> inline void accessFieldW(OMField *field)
{
    auto data = stackTopAccessW<Tvalue>(true);
    auto obj = static_cast<OMOOPDesc *>(stackTopAccess<void *>(true));
    *reinterpret_cast<Ttarget *>(&obj->data[field->offset]) = data;
}

inline void accessField(OMField *field)
{
    int p = 0;
    auto res = bytecode::descriptor::decodeTypeTo(field->desc, &p);
    switch (res.type)
    {
    case bytecode::descriptor::Byte:
        accessFieldI<jbyte, jint>(field);
        break;
    case bytecode::descriptor::Boolean:
        accessFieldI<jboolean, jint>(field);
        break;
    case bytecode::descriptor::Char:
        accessFieldI<jchar, jint>(field);
        break;
    case bytecode::descriptor::Short:
        accessFieldI<jshort, jint>(field);
        break;
    case bytecode::descriptor::Int:
        accessFieldI<jint, jint>(field);
        break;
    case bytecode::descriptor::Float:
        accessFieldI<jfloat, jfloat>(field);
        break;
    case bytecode::descriptor::Long:
        accessFieldW<jlong, jlong>(field);
        break;
    case bytecode::descriptor::Double:
        accessFieldW<jdouble, jdouble>(field);
        break;
    case bytecode::descriptor::Array:
    case bytecode::descriptor::Void:
    case bytecode::descriptor::Reference:
        auto data = stackTopAccess<void *>();
        stackPop();
        auto obj = static_cast<OMOOPDesc *>(stackTopAccess<void *>());
        stackPop();
        auto target = &obj->data[field->offset];
        if (obj->klass->heap->ptrCompEnabled())
        {
            *reinterpret_cast<uint32_t *>(target) = obj->klass->heap->compressPtr(data);
        }
        else
        {
            *reinterpret_cast<void **>(target) = data;
        }
        break;
    }
}

template <typename Ttarget, typename Tvalue> inline void accessFieldStaticI(OMField *field)
{
    auto data = stackTopAccess<Tvalue>();
    stackPop();
    *reinterpret_cast<Ttarget *>(
        static_cast<void *>(static_cast<uint8_t *>(field->klass->staticBlock) + field->offset)) = data;
}

template <typename Ttarget, typename Tvalue> inline void accessFieldStaticW(OMField *field)
{
    auto data = stackTopAccessW<Tvalue>();
    stackPopW();
    *reinterpret_cast<Ttarget *>(
        static_cast<void *>(static_cast<uint8_t *>(field->klass->staticBlock) + field->offset)) = data;
}

inline void accessFieldStatic(OMField *field)
{
    int p = 0;
    auto res = bytecode::descriptor::decodeTypeTo(field->desc, &p);

    switch (res.type)
    {
    case bytecode::descriptor::Byte:
        accessFieldStaticI<jbyte, jint>(field);
        break;
    case bytecode::descriptor::Boolean:
        accessFieldStaticI<jboolean, jint>(field);
        break;
    case bytecode::descriptor::Char:
        accessFieldStaticI<jchar, jint>(field);
        break;
    case bytecode::descriptor::Short:
        accessFieldStaticI<jshort, jint>(field);
        break;
    case bytecode::descriptor::Int:
        accessFieldStaticI<jint, jint>(field);
        break;
    case bytecode::descriptor::Float:
        accessFieldStaticI<jfloat, jfloat>(field);
        break;
    case bytecode::descriptor::Long:
        accessFieldStaticW<jlong, jlong>(field);
        break;
    case bytecode::descriptor::Double:
        accessFieldStaticW<jdouble, jdouble>(field);
        break;
    case bytecode::descriptor::Array:
    case bytecode::descriptor::Void:
    case bytecode::descriptor::Reference:
        auto obj = static_cast<OMOOPDesc *>(stackTopAccess<void *>());
        stackPop();
        auto target = static_cast<void *>(static_cast<uint8_t *>(field->klass->staticBlock) + field->offset);
        if (obj->klass->heap->ptrCompEnabled())
        {
            *reinterpret_cast<uint32_t *>(target) = obj->klass->heap->compressPtr(obj);
        }
        else
        {
            *reinterpret_cast<void **>(target) = obj;
        }
        break;
    }
}

template <typename Ttarget, typename Tvalue> void fetchFieldI(OMField *field)
{
    auto obj = stackTopAccess<OMOOPDesc *>(true);
    stackPushAccess<Tvalue>(*static_cast<Ttarget *>(static_cast<void *>(&obj->data[field->offset])));
}

template <typename Ttarget, typename Tvalue> void fetchFieldW(OMField *field)
{
    auto obj = stackTopAccess<OMOOPDesc *>(true);
    stackPushAccessW<Tvalue>(*static_cast<Ttarget *>(static_cast<void *>(&obj->data[field->offset])));
}

inline void fetchField(OMField *field)
{
    int p = 0;
    auto res = bytecode::descriptor::decodeTypeTo(field->desc, &p);

    switch (res.type)
    {
    case bytecode::descriptor::Byte:
        fetchFieldI<jbyte, jint>(field);
        break;
    case bytecode::descriptor::Boolean:
        fetchFieldI<jboolean, jint>(field);
        break;
    case bytecode::descriptor::Char:
        fetchFieldI<jchar, jint>(field);
        break;
    case bytecode::descriptor::Short:
        fetchFieldI<jshort, jint>(field);
        break;
    case bytecode::descriptor::Int:
        fetchFieldI<jint, jint>(field);
        break;
    case bytecode::descriptor::Float:
        fetchFieldI<jfloat, jfloat>(field);
        break;
    case bytecode::descriptor::Long:
        fetchFieldW<jlong, jlong>(field);
        break;
    case bytecode::descriptor::Double:
        fetchFieldW<jdouble, jdouble>(field);
        break;
    case bytecode::descriptor::Array:
    case bytecode::descriptor::Void:
    case bytecode::descriptor::Reference:
        auto h = currentThread.currentFrame->method->klass->heap;
        auto obj = stackTopAccess<OMOOPDesc *>(true);
        auto target = &obj->data[field->offset];
        if (h->ptrCompEnabled())
        {
            stackPushAccess<void *>(h->decompressPtr(*reinterpret_cast<uint32_t *>(target)));
        }
        else
        {
            stackPushAccess<void *>(*reinterpret_cast<void **>(target));
        }
        break;
    }
}

template <typename Ttarget, typename Tvalue> void fetchFieldStaticI(OMField *field)
{
    stackPushAccess<Tvalue>(*static_cast<Ttarget *>(
        static_cast<void *>(static_cast<uint8_t *>(field->klass->staticBlock) + field->offset)));
}

template <typename Ttarget, typename Tvalue> void fetchFieldStaticW(OMField *field)
{
    stackPushAccessW<Tvalue>(*static_cast<Ttarget *>(
        static_cast<void *>(static_cast<uint8_t *>(field->klass->staticBlock) + field->offset)));
}

inline void fetchFieldStatic(OMField *field)
{
    int p = 0;
    auto res = bytecode::descriptor::decodeTypeTo(field->desc, &p);

    switch (res.type)
    {
    case bytecode::descriptor::Byte:
        fetchFieldStaticI<jbyte, jint>(field);
        break;
    case bytecode::descriptor::Boolean:
        fetchFieldStaticI<jboolean, jint>(field);
        break;
    case bytecode::descriptor::Char:
        fetchFieldStaticI<jchar, jint>(field);
        break;
    case bytecode::descriptor::Short:
        fetchFieldStaticI<jshort, jint>(field);
        break;
    case bytecode::descriptor::Int:
        fetchFieldStaticI<jint, jint>(field);
        break;
    case bytecode::descriptor::Float:
        fetchFieldStaticI<jfloat, jfloat>(field);
        break;
    case bytecode::descriptor::Long:
        fetchFieldStaticW<jlong, jlong>(field);
        break;
    case bytecode::descriptor::Double:
        fetchFieldStaticW<jdouble, jdouble>(field);
        break;
    case bytecode::descriptor::Array:
    case bytecode::descriptor::Void:
    case bytecode::descriptor::Reference:
        auto h = currentThread.currentFrame->method->klass->heap;
        auto target = static_cast<void *>(static_cast<uint8_t *>(field->klass->staticBlock) + field->offset);
        if (h->ptrCompEnabled())
        {
            stackPushAccess<void *>(h->decompressPtr(*reinterpret_cast<uint32_t *>(target)));
        }
        else
        {
            stackPushAccess<void *>(*reinterpret_cast<void **>(target));
        }
        break;
    }
}

inline std::string methodName(OMMethod *m)
{
    return fmt::format("{}.{}{}", m->klass->name, m->name, m->desc);
}

inline std::string currentPosition()
{
    auto me = currentThread.currentFrame->method;
    return fmt::format("{}.{}{} + {}", me->klass->name, me->name, me->desc,
                       static_cast<size_t>(currentThread.pc - me->code));
}

} // namespace openminecraft::vm::pixeltower::v0

#endif
