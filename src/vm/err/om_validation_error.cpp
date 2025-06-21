#include "openminecraft/vm/err/om_validation_error.hpp"
#include "fmt/format.h"

namespace openminecraft::vm::err
{
OMValidationError::OMValidationError(ValidationState state, std::string reason, std::string additional)
    : state(state), reason(reason), additional(additional)
{
}

OMValidationError::OMValidationError() : state(Unknown), reason(""), additional("")
{
}

const char *OMValidationError::what() const throw()
{
    auto str = new std::string(fmt::format("{} -> {}", reason, additional));
    return str->c_str();
}
} // namespace openminecraft::vm::err