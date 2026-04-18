#ifndef OM_MEM_SAFEREAD_HPP
#define OM_MEM_SAFEREAD_HPP

#include <cstdint>
#include <optional>
namespace openminecraft::mem
{
std::optional<uint8_t> safeRead(void *p);
}

#endif
