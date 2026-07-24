#ifndef OM_DEMIURGE_STYLES_HPP
#define OM_DEMIURGE_STYLES_HPP

#include <any>
#include <iostream>
#include <string>
#include <unordered_map>
namespace openminecraft::renderer::common::demiurge
{
class OMDemiurgeStyles
{
  public:
    using iterator = std::unordered_map<std::string, std::any>::iterator;
    using const_iterator = std::unordered_map<std::string, std::any>::const_iterator;

    auto begin() -> iterator
    {
        return styles.begin();
    }
    auto end() -> iterator
    {
        return styles.end();
    }
    auto begin() const -> const_iterator
    {
        return styles.begin();
    }
    auto end() const -> const_iterator
    {
        return styles.end();
    }
    auto cbegin() const -> const_iterator
    {
        return styles.cbegin();
    }
    auto cend() const -> const_iterator
    {
        return styles.cend();
    }
    OMDemiurgeStyles() = default;
    ~OMDemiurgeStyles() = default;

    auto put(std::string s, std::any t)
    {
        styles[s] = t;
        modified = true;
    }
    template <typename T> auto get(std::string s) -> T
    {
        return std::any_cast<T>(styles[s]);
    }

    auto isModified() -> bool
    {
        return modified;
    }
    auto solve()
    {
        modified = false;
    }

  private:
    bool modified = true;
    std::unordered_map<std::string, std::any> styles;
};
} // namespace openminecraft::renderer::common::demiurge

#endif
