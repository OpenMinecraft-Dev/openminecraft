#ifndef OM_VALIDATION_ERROR_HPP
#define OM_VALIDATION_ERROR_HPP

#include <exception>
#include <string>

namespace openminecraft::vm::err
{
enum OMValidationState
{
    Unknown,
    Loading,
    ConstantPool,
    ClassLoader,
    Instructions
};

class OMValidationError : public std::exception
{
  private:
    OMValidationState state;
    std::string reason;
    std::string additional;

  public:
    OMValidationError(OMValidationState, std::string, std::string);
    OMValidationError();
    virtual const char *what() const throw();
};
} // namespace openminecraft::vm::err

#endif