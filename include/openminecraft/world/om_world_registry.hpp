#ifndef OM_WORLD_REGISTRY_HPP
#define OM_WORLD_REGISTRY_HPP

#include <string>
#include <unordered_map>
namespace openminecraft::world
{
template <typename T> class OMWorldRegistry
{
  public:
    OMWorldRegistry() = default;
    ~OMWorldRegistry() = default;

    void registerItem(std::string name, T &item)
    {
        nameToId[std::move(name)] = nextId;
        idToRegistry[nextId] = item;
        ++nextId;
    }
    auto id(std::string name) -> uint32_t
    {
        return nameToId[std::move(name)];
    }
    auto getRegistry(std::string name) -> T &
    {
        return idToRegistry[nameToId[std::move(name)]];
    }

  private:
    uint32_t nextId = 0;
    std::unordered_map<std::string, uint32_t> nameToId;
    std::unordered_map<uint32_t, T> idToRegistry;
};

} // namespace openminecraft::world

#endif // OM_WORLD_REGISTRY_HPP