#include "openminecraft/vm/impl/om_impl_throwable.hpp"

namespace openminecraft::vm::impl
{
std::any java_lang_Throwable_fillInStackTrace(pixeltower::v0::OMPixelTower *tower, std::any *d)
{
    std::cout << "filling for " << std::any_cast<void *>(d[0]) << std::endl;
    return nullptr;
}
} // namespace openminecraft::vm::impl