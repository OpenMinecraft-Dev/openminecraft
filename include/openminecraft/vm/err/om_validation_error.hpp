#ifndef OM_VALIDATION_ERROR
#define OM_VALIDATION_ERROR

#include <string>

namespace openminecraft::vm::err
{
enum ValidationState
{
    Unknown,
    Loading,
    ConstantPool,
    Instructions
};

class OMValidationError
{
  private:
    ValidationState state;
    std::string reason;
    std::string additional;

  public:
    OMValidationError(ValidationState, std::string, std::string);
    OMValidationError();
    std::string what() const;
};
} // namespace openminecraft::vm::err

#endif