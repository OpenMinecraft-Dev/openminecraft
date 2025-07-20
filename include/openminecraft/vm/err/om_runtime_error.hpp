#ifndef OM_RUNTIME_ERROR_HPP
#define OM_RUNTIME_ERROR_HPP

#include <exception>

namespace openminecraft::vm::err
{
class OMRuntimeError : public std::exception
{
  private:
    void *errInstance;

  public:
    OMRuntimeError(void *errInstance);
    virtual const char *what() const throw();
};
} // namespace openminecraft::vm::err

#endif