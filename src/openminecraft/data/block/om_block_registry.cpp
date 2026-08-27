#include "openminecraft-shell/data/block/om_block.hpp"
#include "openminecraft-shell/data/block/om_block_registery.hpp"

namespace openminecraftshell::data::block
{
OMWorldRegistry<OMIdentifier, OMBlock> blockRegistery;

void registerBlocks()
{
    blockRegistery.registerItem(OMIdentifier("minecraft:air"), OMBlock().isSoild(false));
    blockRegistery.registerItem(OMIdentifier("minecraft:stone"), OMBlock().isSoild(true));
    blockRegistery.registerItem(OMIdentifier("minecraft:cobblestone"), OMBlock().isSoild(true));
    blockRegistery.registerItem(OMIdentifier("minecraft:coal_ore"), OMBlock().isSoild(true));
    blockRegistery.registerItem(OMIdentifier("minecraft:iron_ore"), OMBlock().isSoild(true));
    blockRegistery.registerItem(OMIdentifier("minecraft:dirt"), OMBlock().isSoild(true));
    blockRegistery.registerItem(OMIdentifier("minecraft:copper_ore"), OMBlock().isSoild(true));
    blockRegistery.registerItem(OMIdentifier("minecraft:tall_grass"), OMBlock().isSoild(false).propHalf());
    blockRegistery.registerItem(
        OMIdentifier("minecraft:cherry_stairs"),
        OMBlock()
            .isSoild(false)
            .propFacing()
            .prop("half", {"top", "bottom"})
            .prop("shape", {"straight", "inner_left", "inner_right", "outer_left", "outer_right"})
            .propWaterlogged());
    blockRegistery.registerItem(OMIdentifier("minecraft:cherry_door"),
                                OMBlock().isSoild(false).propFacing().propHalf().propHinge().propOpen().propOpen());
    blockRegistery.registerItem(
        OMIdentifier("minecraft:cherry_hanging_sign"),
        OMBlock().isSoild(false).prop("attached", {"true", "false"}).propRotation().propWaterlogged());
    blockRegistery.registerItem(OMIdentifier("minecraft:cherry_button"),
                                OMBlock().isSoild(false).propFace().propFacing().propPowered());
    blockRegistery.registerItem(OMIdentifier("minecraft:cherry_shelf"),
                                OMBlock().isSoild(false).propFacing().propPowered().prop(
                                    "side_chain", {"unconnected", "left", "right", "center"}));
    blockRegistery.registerItem(OMIdentifier("minecraft:grass_block"), OMBlock().isSoild(true).propSnowy());
    blockRegistery.registerItem(
        OMIdentifier("minecraft:cherry_fence_gate"),
        OMBlock().isSoild(false).propFacing().propInWall().propOpen().propPowered().propWaterlogged());
    blockRegistery.registerItem(
        OMIdentifier("minecraft:rail"),
        OMBlock().isSoild(false).prop("shape", {"ascending_east", "ascending_north", "ascending_east", "ascending_west",
                                                "east_west", "north_east", "north_south", "north_west", "south_east",
                                                "south_west"}));
    blockRegistery.registerItem(OMIdentifier("minecraft:blue_stained_glass"),
                                OMBlock().isSoild(false).isTranslucent(true));
}
} // namespace openminecraftshell::data::block
