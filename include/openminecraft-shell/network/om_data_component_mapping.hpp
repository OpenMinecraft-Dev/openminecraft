#ifndef OM_DATA_COMPONENT_MAPPING_HPP
#define OM_DATA_COMPONENT_MAPPING_HPP

#include <cstdint>
#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>

namespace openminecraft::shell::network
{
// INFO: the data_component_type registry is a static registry that servers do not send, so the
// id -> name mapping is version specific. Each protocol transformer owns its own mapping instance
// (no singleton, so several connections with different versions can coexist).
class OMDataComponentMapping
{
  public:
    auto setNames(std::vector<std::string> names) -> void
    {
        ordered = std::move(names);
        index.clear();

        for (std::size_t i = 0; i < ordered.size(); ++i)
        {
            index.emplace(ordered[i], static_cast<int32_t>(i));
        }
    }

    [[nodiscard]] auto name(int32_t id) const -> std::string_view
    {
        if (id < 0 || static_cast<std::size_t>(id) >= ordered.size())
        {
            return {};
        }

        return ordered[static_cast<std::size_t>(id)];
    }

    [[nodiscard]] auto size() const -> std::size_t
    {
        return ordered.size();
    }

  private:
    std::vector<std::string> ordered;
    std::unordered_map<std::string, int32_t> index;
};
} // namespace openminecraft::shell::network

#endif
