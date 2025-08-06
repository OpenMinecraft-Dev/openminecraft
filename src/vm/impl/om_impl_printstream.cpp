#include "openminecraft/vm/impl/om_impl_printstream.hpp"
#include <iostream>

namespace openminecraft::vm::impl
{
std::any vmstd_internal_SystemPrintStream_println(std::any *)
{
    std::cout << "printing!" << std::endl;
    *(int *)nullptr = 0;
    return nullptr;
}
} // namespace openminecraft::vm::impl