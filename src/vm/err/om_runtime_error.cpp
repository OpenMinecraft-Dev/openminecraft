#include "openminecraft/vm/err/om_runtime_error.hpp"
#include "fmt/format.h"

namespace openminecraft::vm::err
{
OMRuntimeError::OMRuntimeError(void *errInstance) : errInstance(errInstance)
{
}
const char *OMRuntimeError::what() const throw()
{
    auto i = new std::string(fmt::format("runtime error thrown with instance {}", errInstance));
    return i->c_str();
}
} // namespace openminecraft::vm::err