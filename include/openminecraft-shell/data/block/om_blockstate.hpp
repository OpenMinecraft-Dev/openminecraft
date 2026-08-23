#ifndef OM_BLOCKSTATE_HPP
#define OM_BLOCKSTATE_HPP

#include <initializer_list>
#include <vector>
#include <string>
#include <unordered_map>
#include <utility>
#include <algorithm>
namespace openminecraftshell::data::block
{
class OMBlockState
{
  public:
    OMBlockState(std::string props)
    {
        int current = 0;
        std::string::size_type next;
        while ((next = props.find_first_of(",", current)) != std::string::npos)
        {
            auto sep = props.find("=", current);
            if (sep > next)
            {
                current = next + 1;
                continue;
            }

            auto key = props.substr(current, sep - current);
            auto value = props.substr(sep + 1, next - sep - 1);
            properties[key] = value;

            current = next + 1;
        }

        if (current < props.size())
        {
            auto sep = props.find("=", current);

            auto key = props.substr(current, sep - current);
            auto value = props.substr(sep + 1, next - sep - 1);
            properties[key] = value;
        }
    }
    OMBlockState(std::initializer_list<std::pair<std::string, std::string>> props)
    {
        for (auto &p : props)
        {
            properties[p.first] = p.second;
        }
    }
    OMBlockState(std::unordered_map<std::string, std::string> properties)
    {
        this->properties = properties;
    }
    ~OMBlockState()
    {
    }

    [[nodiscard]] auto hash() const -> std::size_t
    {
        std::vector<std::pair<std::string, std::string>> sortedProps(properties.begin(), properties.end());
        std::sort(sortedProps.begin(), sortedProps.end());

        std::size_t seed = 0;
        for (const auto &kv : sortedProps)
        {
            std::size_t keyHash = std::hash<std::string>{}(kv.first);
            std::size_t valueHash = std::hash<std::string>{}(kv.second);

            seed ^= keyHash + 0x9e3779b9 + (seed << 6) + (seed >> 2);
            seed ^= valueHash + 0x9e3779b9 + (seed << 6) + (seed >> 2);
        }
        return seed;
    }

    auto operator==(const OMBlockState o) const -> bool
    {
        return hash() == o.hash();
    }

    std::unordered_map<std::string, std::string> properties;
};
} // namespace openminecraftshell::data::block

namespace std
{
template <> struct hash<openminecraftshell::data::block::OMBlockState>
{
    auto operator()(const openminecraftshell::data::block::OMBlockState &s) const noexcept -> std::size_t
    {
        return s.hash();
    }
};
} // namespace std

#endif
