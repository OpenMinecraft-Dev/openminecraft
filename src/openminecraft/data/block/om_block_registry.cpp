#include "openminecraft-shell/data/block/om_block.hpp"
#include "openminecraft-shell/data/block/om_block_registry.hpp"
#include "openminecraft-shell/data/om_identifier.hpp"
#include <memory>

namespace openminecraft::shell::data::block
{
OMWorldRegistry<OMIdentifier, std::shared_ptr<OMBlock>> blockRegistry;

void registerBlocks()
{
    blockRegistry.registerItem(OMIdentifier("minecraft:air"), std::make_shared<OMBlock>()->isSolid(false));
    blockRegistry.registerItem(OMIdentifier("minecraft:stone"), std::make_shared<OMBlock>()->isSolid(true));
    blockRegistry.registerItem(OMIdentifier("minecraft:cobblestone"), std::make_shared<OMBlock>()->isSolid(true));
    blockRegistry.registerItem(OMIdentifier("minecraft:coal_ore"), std::make_shared<OMBlock>()->isSolid(true));
    blockRegistry.registerItem(OMIdentifier("minecraft:iron_ore"), std::make_shared<OMBlock>()->isSolid(true));
    blockRegistry.registerItem(OMIdentifier("minecraft:dirt"), std::make_shared<OMBlock>()->isSolid(true));
    blockRegistry.registerItem(OMIdentifier("minecraft:copper_ore"), std::make_shared<OMBlock>()->isSolid(true));
    blockRegistry.registerItem(OMIdentifier("minecraft:tall_grass"),
                                std::make_shared<OMBlock>()->isSolid(false)->propHalf());
    blockRegistry.registerItem(
        OMIdentifier("minecraft:cherry_stairs"),
        std::make_shared<OMBlock>()
            ->isSolid(false)
            ->propFacing()
            ->prop("half", {"top", "bottom"})
            ->prop("shape", {"straight", "inner_left", "inner_right", "outer_left", "outer_right"})
            ->propWaterlogged());
    blockRegistry.registerItem(
        OMIdentifier("minecraft:cherry_door"),
        std::make_shared<OMBlock>()->isSolid(false)->propFacing()->propHalf()->propHinge()->propOpen()->propOpen());
    blockRegistry.registerItem(OMIdentifier("minecraft:cherry_hanging_sign"), std::make_shared<OMBlock>()
                                                                                   ->isSolid(false)
                                                                                   ->prop("attached", {"true", "false"})
                                                                                   ->propRotation()
                                                                                   ->propWaterlogged());
    blockRegistry.registerItem(OMIdentifier("minecraft:cherry_button"),
                                std::make_shared<OMBlock>()->isSolid(false)->propFace()->propFacing()->propPowered());
    blockRegistry.registerItem(OMIdentifier("minecraft:cherry_shelf"),
                                std::make_shared<OMBlock>()->isSolid(false)->propFacing()->propPowered()->prop(
                                    "side_chain", {"unconnected", "left", "right", "center"}));
    blockRegistry.registerItem(OMIdentifier("minecraft:grass_block"),
                                std::make_shared<OMBlock>()->isSolid(true)->propSnowy());
    blockRegistry.registerItem(OMIdentifier("minecraft:cherry_fence_gate"), std::make_shared<OMBlock>()
                                                                                 ->isSolid(false)
                                                                                 ->propFacing()
                                                                                 ->propInWall()
                                                                                 ->propOpen()
                                                                                 ->propPowered()
                                                                                 ->propWaterlogged());
    blockRegistry.registerItem(
        OMIdentifier("minecraft:rail"),
        std::make_shared<OMBlock>()->isSolid(false)->prop(
            "shape", {"ascending_east", "ascending_north", "ascending_east", "ascending_west", "east_west",
                      "north_east", "north_south", "north_west", "south_east", "south_west"}));
    blockRegistry.registerItem(OMIdentifier("minecraft:white_stained_glass"),
                                std::make_shared<OMTransparentBlock>()->isSolid(false)->isTranslucent(true));
    blockRegistry.registerItem(OMIdentifier("minecraft:glass"),
                                std::make_shared<OMTransparentBlock>()->isSolid(false));
    blockRegistry.registerItem(OMIdentifier("minecraft:stone_pressure_plate"),
                                std::make_shared<OMBlock>()->isSolid(false)->propPowered());
    blockRegistry.registerItem(
        OMIdentifier("minecraft:water"),
        std::make_shared<OMBlock>()
            ->isFluid(true)
            ->isTranslucent(true)
            ->prop("level", {"0", "1", "2", "3", "4", "5", "6", "7", "8", "9", "10", "11", "12", "13", "14", "15"})
            ->prop("falling", {"true", "false"}));
    blockRegistry.registerItem(
        OMIdentifier("minecraft:lava"),
        std::make_shared<OMBlock>()
            ->isFluid(true)
            ->prop("level", {"0", "1", "2", "3", "4", "5", "6", "7", "8", "9", "10", "11", "12", "13", "14", "15"})
            ->prop("falling", {"true", "false"}));
}
} // namespace openminecraft::shell::data::block
