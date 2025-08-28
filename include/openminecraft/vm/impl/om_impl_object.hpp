#ifndef OM_IMPL_OBJECT_HPP
#define OM_IMPL_OBJECT_HPP
#include "openminecraft/vm/pixeltower/v0/om_pixeltower.hpp"

#include <any>

namespace openminecraft::vm::impl
{
std::any java_lang_Object_hashCode(pixeltower::v0::OMPixelTower *, std::any *);
std::any java_lang_Object_getClass(pixeltower::v0::OMPixelTower *, std::any *);
}

#endif