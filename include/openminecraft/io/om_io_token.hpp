#ifndef OM_IO_TOKEN
#define OM_IO_TOKEN

#include <string>
#include <type_traits>
#include <utility>

namespace openminecraft::io
{
template <typename T> class OMToken
{
  public:
    OMToken(T type, std::string content) : type(type), content(std::move(content))
    {
        static_assert(std::is_enum_v<T>, "Token type must be a enum!");
    }

    const T type;
    const std::string content;
};
} // namespace openminecraft::io

#endif
