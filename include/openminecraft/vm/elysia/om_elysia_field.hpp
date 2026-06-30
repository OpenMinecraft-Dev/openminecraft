#ifndef OM_ELYSIA_FIELD_HPP
#define OM_ELYSIA_FIELD_HPP

#include <cstdint>
#include <limits>
#include <openminecraft/specs/classfile/om_classfile.hpp>

namespace openminecraft::vm::elysia
{
constexpr uint32_t fieldOffsetUnknown = std::numeric_limits<uint32_t>::max();
class OMElysiaInstanceKlass;
class OMElysiaField
{
  public:
    OMElysiaInstanceKlass *klass;
    char *name;
    char *desc;
    uint16_t accessFlag;

    uint32_t offset = fieldOffsetUnknown;

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
