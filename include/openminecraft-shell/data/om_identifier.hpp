#ifndef OM_IDENTIFIER_HPP
#define OM_IDENTIFIER_HPP

#include <string>

namespace openminecraftshell::data
{
class OMIdentifier
{
  public:
    OMIdentifier(std::string np, std::string path);
    OMIdentifier(std::string);
    OMIdentifier() = default;

    auto toPath() -> std::string;

    std::string namesp;
    std::string path;
};

inline auto operator==(const OMIdentifier &lhs, const OMIdentifier &rhs) -> bool
{
    return lhs.namesp == rhs.namesp && lhs.path == rhs.path;
}

inline auto operator!=(const OMIdentifier &lhs, const OMIdentifier &rhs) -> bool
{
    return !(lhs == rhs);
}

inline auto operator<(const OMIdentifier &lhs, const OMIdentifier &rhs) -> bool
{
    if (lhs.namesp != rhs.namesp)
        return lhs.namesp < rhs.namesp;
    return lhs.path < rhs.path;
}
} // namespace openminecraftshell::data

namespace std
{
template <> struct hash<openminecraftshell::data::OMIdentifier>
{
    auto operator()(const openminecraftshell::data::OMIdentifier &id) const noexcept -> std::size_t
    {
        std::size_t h1 = std::hash<std::string>{}(id.namesp);
        std::size_t h2 = std::hash<std::string>{}(id.path);
        return h1 ^ (h2 << 1);
    }
};
} // namespace std

#endif