#include "openminecraft-shell/data/om_identifier.hpp"
#include <algorithm>
#include <cctype>

namespace openminecraftshell::data
{
static auto toLower(std::string s) -> std::string
{
    std::transform(s.begin(), s.end(), s.begin(), [](unsigned char c) { return std::tolower(c); });
    return s;
}

OMIdentifier::OMIdentifier(std::string np, std::string p) : namesp(toLower(std::move(np))), path(toLower(std::move(p)))
{
    if (namesp.empty())
        namesp = "minecraft";
}

OMIdentifier::OMIdentifier(std::string full)
{
    full = toLower(std::move(full));

    size_t colonPos = full.find(':');
    if (colonPos != std::string::npos)
    {
        namesp = full.substr(0, colonPos);
        path = full.substr(colonPos + 1);
    }
    else
    {
        namesp = "minecraft";
        path = std::move(full);
    }

    if (namesp.empty())
        namesp = "minecraft";
}

auto OMIdentifier::toPath() -> std::string
{
    return namesp + ":" + path;
}

} // namespace openminecraftshell::data