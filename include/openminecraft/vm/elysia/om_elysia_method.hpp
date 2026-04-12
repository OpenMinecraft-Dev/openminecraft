#ifndef OM_ELYSIA_METHOD
#define OM_ELYSIA_METHOD

#include "openminecraft/vm/classfile/om_class_file.hpp"
#include <cstdint>

namespace openminecraft::vm::elysia
{
class OMElysiaKlass;
struct OMElysiaMethod
{
    OMElysiaKlass *klass;
    char *name;
    char *descriptor;
    uint16_t accessFlag;

    uint32_t localLength;

    uint32_t codeLength;
    uint8_t *code;

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
};
} // namespace openminecraft::vm::elysia

#endif
