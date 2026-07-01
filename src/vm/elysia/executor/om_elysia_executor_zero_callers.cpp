#include "openminecraft/vm/elysia/executor/om_elysia_executor_zero.hpp"
#include "openminecraft/vm/elysia/om_elysia_descriptor.hpp"
#include "openminecraft/vm/elysia/om_elysia_meta.hpp"

namespace openminecraft::vm::elysia::executor
{
void OMElysiaExecutorZero::callVoidFunction(OMElysiaMethod *m, const OMElysiaNativeValue *args)
{
    threadInit();
    int i = 0;
    if (!m->isStatic())
    {
        zeroStackPush(args[i].l ? args[i].l->object : nullptr);
        ++i;
    }

    uint8_t argtypes[255];
    int argcount;
    uint8_t returntype;
    descriptorTypes(m->cachedDescriptor, argtypes, argcount, &returntype);

    auto pos = i;

    for (; i < argcount + pos; i++)
    {
        switch (argtypes[i - pos])
        {
        case argTypeBoolean:
            zeroStackPush<jboolean>(args[i].z);
            continue;
        case argTypeByte:
            zeroStackPush<jbyte>(args[i].b);
            continue;
        case argTypeShort:
            zeroStackPush<jshort>(args[i].s);
            continue;
        case argTypeChar:
            zeroStackPush<jchar>(args[i].c);
            continue;
        case argTypeInt:
            zeroStackPush<jint>(args[i].i);
            continue;
        case argTypeFloat:
            zeroStackPush<jfloat>(args[i].f);
            continue;
        case argTypeLong:
            zeroStackPushW<jlong>(args[i].j);
            continue;
        case argTypeDouble:
            zeroStackPushW<jdouble>(args[i].d);
            continue;
        case argTypeReference:
        case argTypeArray:
            zeroStackPush<OMElysiaOop *>(args[i].l ? args[i].l->object : nullptr);
            continue;
        default:
            throw std::logic_error("unknown arg type");
        }
    }

    execWithState(InsideVM, [&]() { execute(m); });
}

}; // namespace openminecraft::vm::elysia::executor
