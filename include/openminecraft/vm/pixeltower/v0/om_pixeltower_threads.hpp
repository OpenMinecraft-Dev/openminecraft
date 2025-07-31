#ifndef OM_PIXELTOWER_THREADS_HPP
#define OM_PIXELTOWER_THREADS_HPP

#include "openminecraft/binary/om_bin_hash.hpp"
#include "openminecraft/util/om_util_result.hpp"
#include "openminecraft/vm/bytecode/om_bytecode_descriptor.hpp"
#include "openminecraft/vm/err/om_validation_error.hpp"
#include "openminecraft/vm/pixeltower/v0/om_pixeltower_base.hpp"
#include "openminecraft/vm/pixeltower/v0/om_pixeltower_field.hpp"
#include "openminecraft/vm/pixeltower/v0/om_pixeltower_frame.hpp"
#include "openminecraft/vm/pixeltower/v0/om_pixeltower_method.hpp"
#include "openminecraft/vm/pixeltower/v0/om_pixeltower_oop.hpp"
#include <string>
#include <thread>
#include <type_traits>

using namespace openminecraft::binary::hash;
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
        auto raw = *(int64_t *)&data;

        stackPushAccess<jint>(raw & 0xffffffff); // low bits
        stackPushAccess<jint>(raw >> 32);        // high bits
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

        return *(T *)&temp;
    }
}

template <typename Ttarget, typename Tvalue> inline void accessFieldI(OMField *field)
{
    auto data = stackTopAccess<Tvalue>();
    stackPop();
    auto obj = static_cast<OMOOPDesc *>(stackTopAccess<void *>());
    stackPop();
    *reinterpret_cast<Ttarget *>(&obj->data[field->offset]) = data;
}

template <typename Ttarget, typename Tvalue> inline void accessFieldW(OMField *field)
{
    auto data = stackTopAccessW<Tvalue>();
    stackPopW();
    auto obj = static_cast<OMOOPDesc *>(stackTopAccess<void *>());
    stackPop();
    *reinterpret_cast<Ttarget *>(&obj->data[field->offset]) = data;
}

inline void accessField(OMField *field)
{
    int p = 0;
    auto res = bytecode::descriptor::decodeType(field->desc, &p);
    if (res.type == util::Err)
    {
        throw err::OMValidationError{err::Instructions, "unknown field descriptor", field->desc};
    }

    switch (hash_compile_time(res.unwrap().c_str()))
    {
    case "byte"_hash: {
        accessFieldI<jbyte, jint>(field);
        break;
    }
    case "short"_hash: {
        accessFieldI<jshort, jint>(field);
        break;
    }
    case "boolean"_hash: {
        accessFieldI<jboolean, jint>(field);
        break;
    }
    case "int"_hash: {
        accessFieldI<jint, jint>(field);
        break;
    }
    case "float"_hash: {
        accessFieldI<jfloat, jfloat>(field);
        break;
    }
    case "long"_hash: {
        accessFieldW<jlong, jlong>(field);
        break;
    }
    case "double"_hash: {
        accessFieldW<jdouble, jdouble>(field);
        break;
    }
    default: {
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
    auto res = bytecode::descriptor::decodeType(field->desc, &p);
    if (res.type == util::Err)
    {
        throw err::OMValidationError{err::Instructions, "unknown field descriptor", field->desc};
    }

    switch (hash_compile_time(res.unwrap().c_str()))
    {
    case "byte"_hash: {
        accessFieldStaticI<jbyte, jint>(field);
        break;
    }
    case "short"_hash: {
        accessFieldStaticI<jshort, jint>(field);
        break;
    }
    case "boolean"_hash: {
        accessFieldStaticI<jboolean, jint>(field);
        break;
    }
    case "int"_hash: {
        accessFieldStaticI<jint, jint>(field);
        break;
    }
    case "float"_hash: {
        accessFieldStaticI<jfloat, jfloat>(field);
        break;
    }
    case "long"_hash: {
        accessFieldStaticW<jlong, jlong>(field);
        break;
    }
    case "double"_hash: {
        accessFieldStaticW<jdouble, jdouble>(field);
        break;
    }
    default: {
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
}

template <typename Ttarget, typename Tvalue> inline void fetchFieldStaticI(OMField *field)
{
    stackPushAccess<Tvalue>(*reinterpret_cast<Ttarget *>(
        static_cast<void *>(static_cast<uint8_t *>(field->klass->staticBlock) + field->offset)));
}

template <typename Ttarget, typename Tvalue> inline void fetchFieldStaticW(OMField *field)
{
    stackPushAccessW<Tvalue>(*reinterpret_cast<Ttarget *>(
        static_cast<void *>(static_cast<uint8_t *>(field->klass->staticBlock) + field->offset)));
}

inline void fetchFieldStatic(OMField *field)
{
    int p = 0;
    auto res = bytecode::descriptor::decodeType(field->desc, &p);
    if (res.type == util::Err)
    {
        throw err::OMValidationError{err::Instructions, "unknown field descriptor", field->desc};
    }

    switch (hash_compile_time(res.unwrap().c_str()))
    {
    case "byte"_hash: {
        fetchFieldStaticI<jbyte, jint>(field);
        break;
    }
    case "short"_hash: {
        fetchFieldStaticI<jshort, jint>(field);
        break;
    }
    case "boolean"_hash: {
        fetchFieldStaticI<jboolean, jint>(field);
        break;
    }
    case "int"_hash: {
        fetchFieldStaticI<jint, jint>(field);
        break;
    }
    case "float"_hash: {
        fetchFieldStaticI<jfloat, jfloat>(field);
        break;
    }
    case "long"_hash: {
        fetchFieldStaticW<jlong, jlong>(field);
        break;
    }
    case "double"_hash: {
        fetchFieldStaticW<jdouble, jdouble>(field);
        break;
    }
    default: {
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
}

inline std::string methodName(OMMethod *m)
{
    return fmt::format("{}.{}{}", m->klass->name, m->name, m->desc);
}

} // namespace openminecraft::vm::pixeltower::v0

#endif
