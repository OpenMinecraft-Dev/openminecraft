#ifndef OM_ELYSIA_METHOD
#define OM_ELYSIA_METHOD

#include "openminecraft/specs/classfile/om_classfile.hpp"
#include "openminecraft/vm/elysia/interface/om_elysia_interface_defs.hpp"
#include <cstdint>
#include <cstring>

namespace openminecraft::vm::elysia
{
class OMElysiaKlass;
struct OMElysiaMethodExcTable
{
    uint8_t *begin;
    uint8_t *end;
    uint8_t *handler;
    OMElysiaKlass *type;
};
struct OMElysiaMethod
{
    OMElysiaKlass *klass;
    char *name;
    char *descriptor;
    uint16_t accessFlag;

    uint32_t localLength;

    uint32_t codeLength;
    uint8_t *code;

    uint32_t excTableLength;
    OMElysiaMethodExcTable *excTable;

    uint32_t exceptionsLength;
    OMElysiaKlass **exceptions;

#define attr(n)                                                                                                        \
    bool is##n()                                                                                                       \
    {                                                                                                                  \
        return accessFlag & JVM_Acc_##n;                                                                               \
    }
    attr(Static);
    attr(Public);
    attr(Protected);
    attr(Private);
    attr(Native);
    attr(Abstract);

    bool isInit()
    {
        return std::strcmp(name, "<init>") == 0;
    }

    bool isClinit()
    {
        return std::strcmp(name, "<clinit>") == 0;
    }

    bool isSame(OMElysiaMethod *method)
    {
        return std::strcmp(method->name, name) == 0 && std::strcmp(method->descriptor, descriptor) == 0;
    }

    bool isSame(OMElysiaNativeMethod *method)
    {
        return std::strcmp(method->name, name) == 0 && std::strcmp(method->signature, descriptor) == 0;
    }
};
} // namespace openminecraft::vm::elysia

#endif
