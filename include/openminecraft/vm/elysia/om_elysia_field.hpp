#ifndef OM_ELYSIA_FIELD_HPP
#define OM_ELYSIA_FIELD_HPP

#include <cstdint>
#include <limits>

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
};
} // namespace openminecraft::vm::elysia

#endif
