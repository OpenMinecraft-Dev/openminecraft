#ifndef OM_WORLD_REGISTRY_HPP
#define OM_WORLD_REGISTRY_HPP

#include <stdexcept>
#include <unordered_map>
#include <cstdint>
namespace openminecraft::world
{
template <typename K, typename T> class OMWorldRegistry
{
  public:
    OMWorldRegistry() = default;
    ~OMWorldRegistry() = default;

    auto registerItem(K name, T &item) -> T &
    {
        nameToId[std::move(name)] = nextId;
        idToRegistry[nextId] = item;
        ++nextId;

        return idToRegistry[nextId - 1];
    }
    auto id(K name) -> uint32_t
    {
        if (nameToId.count(name))
        {
            return nameToId[std::move(name)];
        }
        else
        {
            throw std::logic_error("key not found!");
        }
    }
    auto getRegistry(K name) -> T &
    {
        return idToRegistry[nameToId[std::move(name)]];
    }

  private:
    uint32_t nextId = 0;

  public:
    std::unordered_map<K, uint32_t> nameToId;
    std::unordered_map<uint32_t, T> idToRegistry;
};

} // namespace openminecraft::world

#endif // OM_WORLD_REGISTRY_HPP