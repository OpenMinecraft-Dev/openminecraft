#ifndef OM_IO_TOKENITER_EXCEPTION_HPP
#define OM_IO_TOKENITER_EXCEPTION_HPP

#include <string>
#include <utility>

namespace openminecraft::io
{
class OMTokenIterException : public std::exception
{
  public:
    OMTokenIterException(std::string s) : msg(std::move(s))
    {
    }

    ~OMTokenIterException() override = default;

    [[nodiscard]] auto what() const noexcept -> const char * override
    {
        return msg.c_str();
    }

  private:
    std::string msg;
};
} // namespace openminecraft::io

#endif
