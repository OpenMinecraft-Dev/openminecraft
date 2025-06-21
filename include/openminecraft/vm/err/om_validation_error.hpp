#ifndef OM_VALIDATION_ERROR
#define OM_VALIDATION_ERROR

#include <exception>
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

class OMValidationError : public std::exception
{
  private:
    ValidationState state;
    std::string reason;
    std::string additional;

  public:
    OMValidationError(ValidationState, std::string, std::string);
    OMValidationError();
    virtual const char *what() const throw();
};
} // namespace openminecraft::vm::err

#endif