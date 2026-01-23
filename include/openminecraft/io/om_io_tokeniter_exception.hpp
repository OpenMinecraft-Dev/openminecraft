#ifndef OM_IO_TOKENITER_EXCEPTION_HPP
#define OM_IO_TOKENITER_EXCEPTION_HPP

#include <stdexcept>
#include <string>

namespace openminecraft::io
{
class OMTokenIterException : public std::exception
{
  public:
    OMTokenIterException(std::string s) : msg(s)
    {
    }

    ~OMTokenIterException() override
    {
    }

    const char *what() const noexcept override
    {
        return msg.c_str();
    }

  private:
    std::string msg;
};
} // namespace openminecraft::io

#endif
