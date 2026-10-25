#ifndef OM_BIOME_HPP
#define OM_BIOME_HPP

#include <cstdint>
#include <memory>

namespace openminecraft::shell::data::biome
{
class OMBiome : public std::enable_shared_from_this<OMBiome>
{
  public:
    OMBiome() = default;

    auto setTemperature(float temperature) -> std::shared_ptr<OMBiome>
    {
        this->temperature = temperature;
        return shared_from_this();
    }
    auto setDownfall(float downfall) -> std::shared_ptr<OMBiome>
    {
        this->downfall = downfall;
        return shared_from_this();
    }
    auto setGrassColor(uint32_t c) -> std::shared_ptr<OMBiome>
    {
        this->grassColor = c;
        return shared_from_this();
    }
    auto setWaterColor(uint32_t c) -> std::shared_ptr<OMBiome>
    {
        this->waterColor = c;
        return shared_from_this();
    }
    auto setFoliageColor(uint32_t c) -> std::shared_ptr<OMBiome>
    {
        this->foliageColor = c;
        return shared_from_this();
    }
    auto setDryFoliageColor(uint32_t c) -> std::shared_ptr<OMBiome>
    {
        this->dryFoliageColor = c;
        return shared_from_this();
    }
    auto setSkyColor(uint32_t c) -> std::shared_ptr<OMBiome>
    {
        this->skyColor = c;
        return shared_from_this();
    }
    auto setFogColor(uint32_t c) -> std::shared_ptr<OMBiome>
    {
        this->fogColor = c;
        return shared_from_this();
    }

    float temperature = 0.8f;
    float downfall = 0.4f;

    uint32_t grassColor = 0x91BD59;
    uint32_t waterColor = 0x3F76E4;
    uint32_t foliageColor = 0x77AB2F;
    uint32_t dryFoliageColor = 0xA37546;

    uint32_t skyColor = 0x78A7FF;
    uint32_t fogColor = 0xC0D8FF;
};
} // namespace openminecraft::shell::data::biome

#endif