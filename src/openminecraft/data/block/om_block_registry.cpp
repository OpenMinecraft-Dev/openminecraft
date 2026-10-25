#include "openminecraft-shell/data/block/om_block.hpp"
#include "openminecraft-shell/data/block/om_block_registry.hpp"
#include "openminecraft-shell/data/om_identifier.hpp"
#include "openminecraft/io/json/om_io_ast_builder_json.hpp"
#include "openminecraft/specs/zlib/om_zlib_inflaterstream.hpp"
#include "openminecraft/vfs/om_vfs_base.hpp"
#include <memory>
#include <unordered_map>

namespace openminecraft::shell::data::block
{
OMWorldRegistry<OMIdentifier, std::shared_ptr<OMBlock>> &blockRegistry =
    *new OMWorldRegistry<OMIdentifier, std::shared_ptr<OMBlock>>();

static std::unordered_map<OMIdentifier, std::unordered_map<std::string, std::vector<std::string>>> blkData;

static void registerBlock(OMIdentifier ident, std::shared_ptr<OMBlock> blk)
{
    for (const auto &p : blkData[ident])
    {
        blk->prop(p.first, p.second);
    }
    blockRegistry.registerItem(ident, blk);
}

void registerBlocks()
{
    io::json::OMJsonAstBuilder bld(std::make_shared<io::json::OMJsonTokenIter>(
        std::make_shared<specs::zlib::OMZlibInflaterStream>(vfs::fsfetch("/bootassets/minecraft/blocks.json.zlib"))));
    auto document = bld.build();

    for (const auto &p : document->getMap())
    {
        auto sub = p.second->getMap();
        auto ident = OMIdentifier(p.first);
        if (sub.count("properties"))
        {
            for (const auto &pp : sub["properties"]->getMap())
            {
                for (const auto &t : pp.second->getArray())
                {
                    blkData[ident][pp.first].push_back(t->getString());
                }
            }
        }
    }

    registerBlock(OMIdentifier("minecraft:air"), std::make_shared<OMBlock>()->isSolid(false));
    registerBlock(OMIdentifier("minecraft:stone"), std::make_shared<OMBlock>()->isSolid(true));
    registerBlock(OMIdentifier("minecraft:cobblestone"), std::make_shared<OMBlock>()->isSolid(true));
    registerBlock(OMIdentifier("minecraft:coal_ore"), std::make_shared<OMBlock>()->isSolid(true));
    registerBlock(OMIdentifier("minecraft:iron_ore"), std::make_shared<OMBlock>()->isSolid(true));
    registerBlock(OMIdentifier("minecraft:dirt"), std::make_shared<OMBlock>()->isSolid(true));
    registerBlock(OMIdentifier("minecraft:copper_ore"), std::make_shared<OMBlock>()->isSolid(true));
    registerBlock(OMIdentifier("minecraft:tall_grass"), std::make_shared<OMBlock>()->isSolid(false));
    registerBlock(OMIdentifier("minecraft:cherry_stairs"), std::make_shared<OMBlock>()->isSolid(false));
    registerBlock(OMIdentifier("minecraft:cherry_door"), std::make_shared<OMBlock>()->isSolid(false));
    registerBlock(OMIdentifier("minecraft:cherry_hanging_sign"), std::make_shared<OMBlock>()->isSolid(false));
    registerBlock(OMIdentifier("minecraft:cherry_button"), std::make_shared<OMBlock>()->isSolid(false));
    registerBlock(OMIdentifier("minecraft:cherry_shelf"), std::make_shared<OMBlock>()->isSolid(false));
    registerBlock(OMIdentifier("minecraft:grass_block"), std::make_shared<OMBlock>()->isSolid(true));
    registerBlock(OMIdentifier("minecraft:cherry_fence_gate"), std::make_shared<OMBlock>()->isSolid(false));
    registerBlock(OMIdentifier("minecraft:rail"), std::make_shared<OMBlock>()->isSolid(false));
    registerBlock(OMIdentifier("minecraft:white_stained_glass"),
                  std::make_shared<OMTransparentBlock>()->isSolid(false)->isTranslucent(true));
    registerBlock(OMIdentifier("minecraft:glass"),
                  std::make_shared<OMTransparentBlock>()->isSolid(false)->isTranslucent(true));
    registerBlock(OMIdentifier("minecraft:stone_pressure_plate"), std::make_shared<OMBlock>()->isSolid(false));
    blockRegistry.registerItem(
        OMIdentifier("minecraft:water"),
        std::make_shared<OMBlock>()
            ->isSolid(false)
            ->isFluid(true)
            ->isTranslucent(true)
            ->prop("level", {"0", "1", "2", "3", "4", "5", "6", "7", "8", "9", "10", "11", "12", "13", "14", "15"})
            ->prop("falling", {"true", "false"}));
    blockRegistry.registerItem(
        OMIdentifier("minecraft:lava"),
        std::make_shared<OMBlock>()
            ->isSolid(false)
            ->isFluid(true)
            ->prop("level", {"0", "1", "2", "3", "4", "5", "6", "7", "8", "9", "10", "11", "12", "13", "14", "15"})
            ->prop("falling", {"true", "false"}));
}
} // namespace openminecraft::shell::data::block
