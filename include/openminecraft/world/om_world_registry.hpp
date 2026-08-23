#ifndef OM_WORLD_REGISTRY_HPP
#define OM_WORLD_REGISTRY_HPP

#include <unordered_map>
#include <cstdint>
namespace openminecraft::world
{
template <typename K, typename T> class OMWorldRegistry
{
  public:
    OMWorldRegistry() = default;
    ~OMWorldRegistry() = default;

    void registerItem(K name, T &item)
    {
        nameToId[std::move(name)] = nextId;
        idToRegistry[nextId] = item;
        ++nextId;
    }
    auto id(K name) -> uint32_t
    {
        return nameToId[std::move(name)];
    }
    auto getRegistry(K name) -> T &
    {
        return idToRegistry[nameToId[std::move(name)]];
    }

  private:
    uint32_t nextId = 0;

  public:
    std::unordered_map<K, uint32_t> nameToId;

  private:
    std::unordered_map<uint32_t, T> idToRegistry;
};

} // namespace openminecraft::world

#endif // OM_WORLD_REGISTRY_HPP