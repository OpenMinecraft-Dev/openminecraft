#include "openminecraft/vm/impl/om_impl_printstream.hpp"
#include "openminecraft/vm/pixeltower/v0/om_pixeltower_base.hpp"
#include <iostream>

namespace openminecraft::vm::impl
{
std::any vmstd_internal_SystemPrintStream_println(std::any *args)
{
    std::cout << std::any_cast<pixeltower::v0::jlong>(args[1]) << std::endl;
    return nullptr;
}
} // namespace openminecraft::vm::impl