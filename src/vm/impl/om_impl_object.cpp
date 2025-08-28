#include "openminecraft/vm/impl/om_impl_object.hpp"

namespace openminecraft::vm::impl
{
std::any java_lang_Object_hashCode(pixeltower::v0::OMPixelTower *, std::any *d)
{
    return static_cast<pixeltower::v0::jint>(reinterpret_cast<size_t>(std::any_cast<void *>(d[0])));
}

std::any java_lang_Object_getClass(pixeltower::v0::OMPixelTower *, std::any *d)
{
    return static_cast<pixeltower::v0::OMOOPDesc *>(std::any_cast<void *>(d[0]))->klass->oop;
}
} // namespace openminecraft::vm::impl