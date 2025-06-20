#include "openminecraft/vm/err/om_validation_error.hpp"
#include "openminecraft/log/om_log_common.hpp"

namespace openminecraft::vm::err
{
OMValidationError::OMValidationError(ValidationState state, std::string reason, std::string additional)
    : state(state), reason(reason), additional(additional)
{
}

OMValidationError::OMValidationError() : state(Unknown), reason(""), additional("")
{
}

std::string OMValidationError::what() const
{
    return fmt::format("{} -> {}", reason, additional);
}
} // namespace openminecraft::vm::err