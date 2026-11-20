#ifndef OM_RUNTIME_ERROR_HPP
#define OM_RUNTIME_ERROR_HPP

#include <exception>

namespace openminecraft::vm::err
{
class OMRuntimeError : public std::exception
{
  public:
    void *errInstance;

    explicit OMRuntimeError(void *errInstance);
    [[nodiscard]] const char *what() const noexcept override;
};
} // namespace openminecraft::vm::err

#endif