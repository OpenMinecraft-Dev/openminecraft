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
    blockRegistery.registerItem(OMIdentifier("minecraft:cherry_stairs"), OMBlock().isSoild(false));
    blockRegistery.registerItem(OMIdentifier("minecraft:cherry_door"), OMBlock().isSoild(false));
    blockRegistery.registerItem(OMIdentifier("minecraft:cherry_hanging_sign"), OMBlock().isSoild(false));
    blockRegistery.registerItem(OMIdentifier("minecraft:cherry_button"), OMBlock().isSoild(false));
    blockRegistery.registerItem(OMIdentifier("minecraft:cherry_shelf"), OMBlock().isSoild(false));
    blockRegistery.registerItem(OMIdentifier("minecraft:grass_block"), OMBlock().isSoild(true));
    blockRegistery.registerItem(OMIdentifier("minecraft:cherry_fence_gate"),
                                OMBlock().isSoild(false).propFacing().propInWall().propOpen().propPowered());
    blockRegistery.registerItem(OMIdentifier("minecraft:rail"), OMBlock().isSoild(false));
}
} // namespace openminecraftshell::data::block
