#ifndef OM_IMPL_THROWABLE_HPP
#define OM_IMPL_THROWABLE_HPP
#include "openminecraft/vm/pixeltower/v0/om_pixeltower.hpp"

namespace openminecraft::vm::impl
{
std::any java_lang_Throwable_fillInStackTrace(pixeltower::v0::OMPixelTower *tower, std::any *);
}

#endif