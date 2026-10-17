#ifndef OM_IDENTIFIER_HPP
#define OM_IDENTIFIER_HPP

#include <string>

namespace openminecraft::shell::data
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
} // namespace openminecraft::shell::data

namespace std
{
template <> struct hash<openminecraft::shell::data::OMIdentifier>
{
    auto operator()(const openminecraft::shell::data::OMIdentifier &id) const noexcept -> std::size_t
    {
        std::size_t namespaceHash = std::hash<std::string>{}(id.namesp);
        std::size_t pathHash = std::hash<std::string>{}(id.path);
        return namespaceHash ^ (pathHash << 1);
    }
};
} // namespace std

#endif