#ifndef OM_MEM_SAFEREAD_HPP
#define OM_MEM_SAFEREAD_HPP

#include <optional>
namespace openminecraft::mem
{
template <typename T> std::optional<T> safeRead(void *p);
} // namespace openminecraft::mem

#endif
