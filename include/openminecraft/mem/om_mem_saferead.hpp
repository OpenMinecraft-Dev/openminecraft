#ifndef OM_MEM_SAFEREAD_HPP
#define OM_MEM_SAFEREAD_HPP

#include <optional>
namespace openminecraft::mem
{
template <typename T> auto safeRead(void *p) -> std::optional<T>;
} // namespace openminecraft::mem

#endif
