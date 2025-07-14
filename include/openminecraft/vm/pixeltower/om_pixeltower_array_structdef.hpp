#ifndef OM_PIXELTOWER_ARRAY_STRUCTDEF_HPP
#define OM_PIXELTOWER_ARRAY_STRUCTDEF_HPP

#include <cstdint>
namespace openminecraft::vm::pixeltower
{
template <typename T> struct OMArray
{
    uint32_t length;
    T *data;
};
} // namespace openminecraft::vm::pixeltower

#endif