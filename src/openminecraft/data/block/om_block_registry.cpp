#include "openminecraft-shell/data/block/om_block.hpp"
#include "openminecraft-shell/data/block/om_block_registery.hpp"
#include "openminecraft-shell/data/om_identifier.hpp"
#include <memory>

namespace openminecraftshell::data::block
{
OMWorldRegistry<OMIdentifier, std::shared_ptr<OMBlock>> blockRegistery;

void registerBlocks()
{
    blockRegistery.registerItem(OMIdentifier("minecraft:air"), std::make_shared<OMBlock>()->isSoild(false));
    blockRegistery.registerItem(OMIdentifier("minecraft:stone"), std::make_shared<OMBlock>()->isSoild(true));
    blockRegistery.registerItem(OMIdentifier("minecraft:cobblestone"), std::make_shared<OMBlock>()->isSoild(true));
    blockRegistery.registerItem(OMIdentifier("minecraft:coal_ore"), std::make_shared<OMBlock>()->isSoild(true));
    blockRegistery.registerItem(OMIdentifier("minecraft:iron_ore"), std::make_shared<OMBlock>()->isSoild(true));
    blockRegistery.registerItem(OMIdentifier("minecraft:dirt"), std::make_shared<OMBlock>()->isSoild(true));
    blockRegistery.registerItem(OMIdentifier("minecraft:copper_ore"), std::make_shared<OMBlock>()->isSoild(true));
    blockRegistery.registerItem(OMIdentifier("minecraft:tall_grass"),
                                std::make_shared<OMBlock>()->isSoild(false)->propHalf());
    blockRegistery.registerItem(
        OMIdentifier("minecraft:cherry_stairs"),
        std::make_shared<OMBlock>()
            ->isSoild(false)
            ->propFacing()
            ->prop("half", {"top", "bottom"})
            ->prop("shape", {"straight", "inner_left", "inner_right", "outer_left", "outer_right"})
            ->propWaterlogged());
    blockRegistery.registerItem(
        OMIdentifier("minecraft:cherry_door"),
        std::make_shared<OMBlock>()->isSoild(false)->propFacing()->propHalf()->propHinge()->propOpen()->propOpen());
    blockRegistery.registerItem(OMIdentifier("minecraft:cherry_hanging_sign"), std::make_shared<OMBlock>()
                                                                                   ->isSoild(false)
                                                                                   ->prop("attached", {"true", "false"})
                                                                                   ->propRotation()
                                                                                   ->propWaterlogged());
    blockRegistery.registerItem(OMIdentifier("minecraft:cherry_button"),
                                std::make_shared<OMBlock>()->isSoild(false)->propFace()->propFacing()->propPowered());
    blockRegistery.registerItem(OMIdentifier("minecraft:cherry_shelf"),
                                std::make_shared<OMBlock>()->isSoild(false)->propFacing()->propPowered()->prop(
                                    "side_chain", {"unconnected", "left", "right", "center"}));
    blockRegistery.registerItem(OMIdentifier("minecraft:grass_block"),
                                std::make_shared<OMBlock>()->isSoild(true)->propSnowy());
    blockRegistery.registerItem(OMIdentifier("minecraft:cherry_fence_gate"), std::make_shared<OMBlock>()
                                                                                 ->isSoild(false)
                                                                                 ->propFacing()
                                                                                 ->propInWall()
                                                                                 ->propOpen()
                                                                                 ->propPowered()
                                                                                 ->propWaterlogged());
    blockRegistery.registerItem(
        OMIdentifier("minecraft:rail"),
        std::make_shared<OMBlock>()->isSoild(false)->prop(
            "shape", {"ascending_east", "ascending_north", "ascending_east", "ascending_west", "east_west",
                      "north_east", "north_south", "north_west", "south_east", "south_west"}));
    blockRegistery.registerItem(OMIdentifier("minecraft:white_stained_glass"),
                                std::make_shared<OMTransparentBlock>()->isSoild(false)->isTranslucent(true));
    blockRegistery.registerItem(OMIdentifier("minecraft:glass"),
                                std::make_shared<OMTransparentBlock>()->isSoild(false));
    blockRegistery.registerItem(OMIdentifier("minecraft:stone_pressure_plate"),
                                std::make_shared<OMBlock>()->isSoild(false)->propPowered());
    blockRegistery.registerItem(
        OMIdentifier("minecraft:water"),
        std::make_shared<OMBlock>()
            ->isFluid(true)
            ->isTranslucent(true)
            ->prop("level", {"0", "1", "2", "3", "4", "5", "6", "7", "8", "9", "10", "11", "12", "13", "14", "15"})
            ->prop("falling", {"true", "false"}));
    blockRegistery.registerItem(
        OMIdentifier("minecraft:lava"),
        std::make_shared<OMBlock>()
            ->isFluid(true)
            ->prop("level", {"0", "1", "2", "3", "4", "5", "6", "7", "8", "9", "10", "11", "12", "13", "14", "15"})
            ->prop("falling", {"true", "false"}));
}
} // namespace openminecraftshell::data::block
