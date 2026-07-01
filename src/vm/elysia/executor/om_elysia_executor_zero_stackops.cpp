#include "openminecraft/vm/elysia/executor/om_elysia_executor_zero.hpp"
#include "openminecraft/vm/elysia/om_elysia_oopmanager.hpp"
#include <cstdint>

namespace openminecraft::vm::elysia::executor
{
uintptr_t zeroStackAlloc(uint64_t len)
{
    auto tc = thisThread.metadata;
    auto v = tc->zero.stackPointer -= len;
    return v;
}

uintptr_t zeroStackPop(uint64_t len)
{
    auto tc = thisThread.metadata;
    auto result = tc->zero.stackPointer;
    tc->zero.stackPointer += len;
    return result;
}

uint16_t zeroCodeFetchArgu16p0(uint8_t *pc)
{
    auto tc = thisThread.metadata;
    return static_cast<uint16_t>(pc[1] << 8) | pc[2];
}

int16_t zeroCodeFetchArgs16p0(uint8_t *pc)
{
    auto tc = thisThread.metadata;
    return static_cast<int16_t>(pc[1] << 8) | pc[2];
}

int32_t zeroCodeFetchArgs32Align(uint8_t *pc, int offset)
{
    auto tc = thisThread.metadata;
    auto pp = pc + 1;
    while (reinterpret_cast<uintptr_t>(pp) % 4)
    {
        ++pp;
    }
    return static_cast<int32_t>(pp[offset * 4] << 24) | static_cast<int32_t>(pp[offset * 4 + 1] << 16) |
           static_cast<int32_t>(pp[offset * 4 + 2] << 8) | pp[offset * 4 + 3];
}

void zeroStackPushFromStatic(OMElysiaField *field, OMElysiaOopManager *oop, OMElysium *world)
{
    switch (*field->desc)
    {
#define accessReadS(f, type, set)                                                                                      \
    case f:                                                                                                            \
        set(*reinterpret_cast<type *>(reinterpret_cast<uintptr_t>(field->klass->staticBlock) + field->offset));        \
        break;
        accessReadS('Z', jboolean, zeroStackPush);
        accessReadS('C', jchar, zeroStackPush);
        accessReadS('S', jshort, zeroStackPush);
        accessReadS('B', jbyte, zeroStackPush);
        accessReadS('I', jint, zeroStackPush);
        accessReadS('F', jfloat, zeroStackPush);
        accessReadS('J', jlong, zeroStackPushW);
        accessReadS('D', jdouble, zeroStackPushW);
    case 'L':
    case '[': {
        zeroStackPush(oop->oopAccessPointerStaticField(field->klass, field->offset));
        break;
    }
    default:
        zeroStackPush(
            *reinterpret_cast<jint *>(reinterpret_cast<uintptr_t>(field->klass->staticBlock) + field->offset));
        break;
    }
}

void zeroStackPopToStatic(OMElysiaField *field, OMElysiaOopManager *oop, OMElysium *world)
{
    switch (*field->desc)
    {
#define accessWriteS(f, type, get)                                                                                     \
    case f:                                                                                                            \
        *reinterpret_cast<type *>(reinterpret_cast<uintptr_t>(field->klass->staticBlock) + field->offset) =            \
            get<type>();                                                                                               \
        break;

        accessWriteS('Z', jboolean, zeroStackPopGet);
        accessWriteS('B', jbyte, zeroStackPopGet);
        accessWriteS('C', jchar, zeroStackPopGet);
        accessWriteS('S', jshort, zeroStackPopGet);
        accessWriteS('I', jint, zeroStackPopGet);
        accessWriteS('F', jfloat, zeroStackPopGet);
        accessWriteS('J', jlong, zeroStackPopWGet);
        accessWriteS('D', jdouble, zeroStackPopWGet);
    case 'L':
    case '[': {
        auto pp = zeroStackPopGet<OMElysiaOop *>();
        oop->oopAccessPointerStaticField(field->klass, field->offset, pp);
        break;
    }
    default:
        *reinterpret_cast<jint *>(reinterpret_cast<uintptr_t>(field->klass->staticBlock) + field->offset) =
            zeroStackPopGet<jint>();
        break;
    }
}

void zeroStackPushFromField(OMElysiaField *field, OMElysiaOopManager *oop, OMElysium *world)
{
    switch (*field->desc)
    {
#define accessRead(f, type, set)                                                                                       \
    case f:                                                                                                            \
        set(*reinterpret_cast<type *>(oop->oopAccessField(zeroStackPopGet<OMElysiaOop *>(), field->offset)));          \
        break;

        accessRead('Z', jboolean, zeroStackPush);
        accessRead('B', jbyte, zeroStackPush);
        accessRead('C', jchar, zeroStackPush);
        accessRead('S', jshort, zeroStackPush);
        accessRead('I', jint, zeroStackPush);
        accessRead('F', jfloat, zeroStackPush);
        accessRead('J', jlong, zeroStackPushW);
        accessRead('D', jdouble, zeroStackPushW);
    case 'L':
    case '[': {
        zeroStackPush(oop->oopAccessPointerField(zeroStackPopGet<OMElysiaOop *>(), field->offset));
        break;
    }
    default:
        zeroStackPush(*reinterpret_cast<jint *>(oop->oopAccessField(zeroStackPopGet<OMElysiaOop *>(), field->offset)));
        break;
    }
}

void zeroStackPopToField(OMElysiaField *field, OMElysiaOopManager *oop, OMElysium *world)
{
    switch (*field->desc)
    {
#define accessWrite(f, type, get)                                                                                      \
    case f: {                                                                                                          \
        auto pp = get<type>();                                                                                         \
        auto obj = zeroStackPopGet<OMElysiaOop *>();                                                                   \
        *reinterpret_cast<type *>(oop->oopAccessField(obj, field->offset)) = pp;                                       \
        break;                                                                                                         \
    }
        accessWrite('Z', jboolean, zeroStackPopGet);
        accessWrite('C', jchar, zeroStackPopGet);
        accessWrite('S', jshort, zeroStackPopGet);
        accessWrite('B', jbyte, zeroStackPopGet);
        accessWrite('I', jint, zeroStackPopGet);
        accessWrite('F', jfloat, zeroStackPopGet);
        accessWrite('J', jlong, zeroStackPopWGet);
        accessWrite('D', jdouble, zeroStackPopWGet);
    case 'L':
    case '[': {
        auto pp = zeroStackPopGet<OMElysiaOop *>();
        oop->oopAccessPointerField(zeroStackPopGet<OMElysiaOop *>(), field->offset, pp);
        break;
    }
    default: {
        auto pp = zeroStackPopGet<jint>();
        *reinterpret_cast<jint *>(oop->oopAccessField(zeroStackPopGet<OMElysiaOop *>(), field->offset)) = pp;
        break;
    }
    }
}
}; // namespace openminecraft::vm::elysia::executor
