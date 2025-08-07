#include "openminecraft/vm/impl/om_impl_printstream.hpp"
#include "openminecraft/vm/pixeltower/v0/om_pixeltower_base.hpp"
#include <iostream>
#include <typeindex>

namespace openminecraft::vm::impl
{
std::any vmstd_internal_SystemPrintStream_println(std::any *args)
{
    auto t = std::type_index(args[1].type());
    if (t.name() == std::type_index(typeid(pixeltower::v0::jlong)).name())
    {
        std::cout << "[stdout] " << std::any_cast<pixeltower::v0::jlong>(args[1]) << std::endl;
    }
    else if (t.name() == std::type_index(typeid(pixeltower::v0::jfloat)).name())
    {
        std::cout << "[stdout] " << std::any_cast<pixeltower::v0::jfloat>(args[1]) << std::endl;
    }
    return nullptr;
}
} // namespace openminecraft::vm::impl