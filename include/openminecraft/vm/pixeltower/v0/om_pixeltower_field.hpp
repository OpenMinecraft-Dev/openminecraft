#ifndef OM_PIXELTOWER_FIELD_HPP
#define OM_PIXELTOWER_FIELD_HPP

#include "openminecraft/vm/pixeltower/v0/om_pixeltower_base.hpp"
#include <string>
namespace openminecraft::vm::pixeltower::v0
{
struct OMField
{
    std::string name;
    std::string desc;
    jint offset;
    AccessFlags accessFlags;
};
} // namespace openminecraft::vm::pixeltower::v0

#endif