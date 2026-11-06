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
    registerBlock(OMIdentifier("minecraft:tall_grass"),
                  std::make_shared<OMBlock>()->isSolid(false)->setColorSource(BiomeGrass));
    registerBlock(OMIdentifier("minecraft:cherry_stairs"),
                  std::make_shared<OMBlock>()->isSolid(false)->setColorSource(BiomeWater));
    registerBlock(OMIdentifier("minecraft:cherry_door"), std::make_shared<OMBlock>()->isSolid(false));
    registerBlock(OMIdentifier("minecraft:cherry_hanging_sign"), std::make_shared<OMBlock>()->isSolid(false));
    registerBlock(OMIdentifier("minecraft:cherry_button"), std::make_shared<OMBlock>()->isSolid(false));
    registerBlock(OMIdentifier("minecraft:cherry_shelf"), std::make_shared<OMBlock>()->isSolid(false));
    registerBlock(OMIdentifier("minecraft:grass_block"),
                  std::make_shared<OMBlock>()->isSolid(true)->setColorSource(BiomeGrass));
    registerBlock(OMIdentifier("minecraft:cherry_fence_gate"), std::make_shared<OMBlock>()->isSolid(false));
    registerBlock(OMIdentifier("minecraft:rail"), std::make_shared<OMBlock>()->isSolid(false));
    registerBlock(OMIdentifier("minecraft:white_stained_glass"),
                  std::make_shared<OMTransparentBlock>()->isSolid(false)->isTranslucent(true));
    registerBlock(OMIdentifier("minecraft:glass"),
                  std::make_shared<OMTransparentBlock>()->isSolid(false)->isTranslucent(true));
    registerBlock(OMIdentifier("minecraft:stone_pressure_plate"), std::make_shared<OMBlock>()->isSolid(false));
    registerBlock(
        OMIdentifier("minecraft:water"),
        std::make_shared<OMBlock>()->isSolid(false)->isFluid(true)->isTranslucent(true)->setColorSource(BiomeWater));
    registerBlock(OMIdentifier("minecraft:lava"), std::make_shared<OMBlock>()->isSolid(false)->isFluid(true));

    registerBlock(OMIdentifier("minecraft:amethyst_block"), std::make_shared<OMBlock>()->isSolid(true));
    registerBlock(OMIdentifier("minecraft:amethyst_cluster"), std::make_shared<OMBlock>()->isSolid(false));
    registerBlock(OMIdentifier("minecraft:andesite"), std::make_shared<OMBlock>()->isSolid(true));
    registerBlock(OMIdentifier("minecraft:azalea"), std::make_shared<OMBlock>()->isSolid(false));
    registerBlock(OMIdentifier("minecraft:bedrock"), std::make_shared<OMBlock>()->isSolid(true));
    registerBlock(OMIdentifier("minecraft:big_dripleaf"), std::make_shared<OMBlock>()->isSolid(false));
    registerBlock(OMIdentifier("minecraft:big_dripleaf_stem"), std::make_shared<OMBlock>()->isSolid(false));
    registerBlock(OMIdentifier("minecraft:brown_mushroom"), std::make_shared<OMBlock>()->isSolid(false));
    registerBlock(OMIdentifier("minecraft:bubble_column"),
                  std::make_shared<OMTransparentBlock>()->isSolid(false)->isTranslucent(true));
    registerBlock(OMIdentifier("minecraft:budding_amethyst"), std::make_shared<OMBlock>()->isSolid(true));
    registerBlock(OMIdentifier("minecraft:calcite"), std::make_shared<OMBlock>()->isSolid(true));
    registerBlock(OMIdentifier("minecraft:cave_air"), std::make_shared<OMBlock>()->isSolid(true));
    registerBlock(OMIdentifier("minecraft:cave_vines"), std::make_shared<OMBlock>()->isSolid(false));
    registerBlock(OMIdentifier("minecraft:cave_vines_plant"), std::make_shared<OMBlock>()->isSolid(false));
    registerBlock(OMIdentifier("minecraft:chest"), std::make_shared<OMBlock>()->isSolid(false));
    registerBlock(OMIdentifier("minecraft:clay"), std::make_shared<OMBlock>()->isSolid(true));
    registerBlock(OMIdentifier("minecraft:deepslate"), std::make_shared<OMBlock>()->isSolid(true));
    registerBlock(OMIdentifier("minecraft:deepslate_coal_ore"), std::make_shared<OMBlock>()->isSolid(true));
    registerBlock(OMIdentifier("minecraft:deepslate_copper_ore"), std::make_shared<OMBlock>()->isSolid(true));
    registerBlock(OMIdentifier("minecraft:deepslate_diamond_ore"), std::make_shared<OMBlock>()->isSolid(true));
    registerBlock(OMIdentifier("minecraft:deepslate_gold_ore"), std::make_shared<OMBlock>()->isSolid(true));
    registerBlock(OMIdentifier("minecraft:deepslate_iron_ore"), std::make_shared<OMBlock>()->isSolid(true));
    registerBlock(OMIdentifier("minecraft:deepslate_lapis_ore"), std::make_shared<OMBlock>()->isSolid(true));
    registerBlock(OMIdentifier("minecraft:deepslate_redstone_ore"), std::make_shared<OMBlock>()->isSolid(true));
    registerBlock(OMIdentifier("minecraft:diamond_ore"), std::make_shared<OMBlock>()->isSolid(true));
    registerBlock(OMIdentifier("minecraft:diorite"), std::make_shared<OMBlock>()->isSolid(true));
    registerBlock(OMIdentifier("minecraft:fern"),
                  std::make_shared<OMBlock>()->isSolid(false)->setColorSource(BiomeGrass));
    registerBlock(OMIdentifier("minecraft:firefly_bush"), std::make_shared<OMBlock>()->isSolid(false));
    registerBlock(OMIdentifier("minecraft:flowering_azalea"), std::make_shared<OMBlock>()->isSolid(false));
    registerBlock(OMIdentifier("minecraft:glow_lichen"), std::make_shared<OMBlock>()->isSolid(false));
    registerBlock(OMIdentifier("minecraft:gold_ore"), std::make_shared<OMBlock>()->isSolid(true));
    registerBlock(OMIdentifier("minecraft:granite"), std::make_shared<OMBlock>()->isSolid(true));
    registerBlock(OMIdentifier("minecraft:gravel"), std::make_shared<OMBlock>()->isSolid(true));
    registerBlock(OMIdentifier("minecraft:ice"),
                  std::make_shared<OMTransparentBlock>()->isSolid(true)->isTranslucent(true));
    registerBlock(OMIdentifier("minecraft:lapis_ore"), std::make_shared<OMBlock>()->isSolid(true));
    registerBlock(OMIdentifier("minecraft:large_amethyst_bud"), std::make_shared<OMBlock>()->isSolid(false));
    registerBlock(OMIdentifier("minecraft:large_fern"),
                  std::make_shared<OMBlock>()->isSolid(false)->setColorSource(BiomeGrass));
    registerBlock(OMIdentifier("minecraft:magma_block"), std::make_shared<OMBlock>()->isSolid(true));
    registerBlock(OMIdentifier("minecraft:medium_amethyst_bud"), std::make_shared<OMBlock>()->isSolid(false));
    registerBlock(OMIdentifier("minecraft:moss_block"), std::make_shared<OMBlock>()->isSolid(true));
    registerBlock(OMIdentifier("minecraft:moss_carpet"), std::make_shared<OMBlock>()->isSolid(false));
    registerBlock(OMIdentifier("minecraft:mossy_cobblestone"), std::make_shared<OMBlock>()->isSolid(true));
    registerBlock(OMIdentifier("minecraft:obsidian"), std::make_shared<OMBlock>()->isSolid(true));
    registerBlock(OMIdentifier("minecraft:raw_copper_block"), std::make_shared<OMBlock>()->isSolid(true));
    registerBlock(OMIdentifier("minecraft:red_mushroom"), std::make_shared<OMBlock>()->isSolid(false));
    registerBlock(OMIdentifier("minecraft:redstone_ore"), std::make_shared<OMBlock>()->isSolid(true));
    registerBlock(OMIdentifier("minecraft:sand"), std::make_shared<OMBlock>()->isSolid(true));
    registerBlock(OMIdentifier("minecraft:sandstone"), std::make_shared<OMBlock>()->isSolid(true));
    registerBlock(OMIdentifier("minecraft:short_grass"),
                  std::make_shared<OMBlock>()->isSolid(false)->setColorSource(BiomeGrass));
    registerBlock(OMIdentifier("minecraft:small_amethyst_bud"), std::make_shared<OMBlock>()->isSolid(false));
    registerBlock(OMIdentifier("minecraft:small_dripleaf"), std::make_shared<OMBlock>()->isSolid(false));
    registerBlock(OMIdentifier("minecraft:smooth_basalt"), std::make_shared<OMBlock>()->isSolid(true));
    registerBlock(OMIdentifier("minecraft:snow"), std::make_shared<OMBlock>()->isSolid(false));
    registerBlock(OMIdentifier("minecraft:spawner"), std::make_shared<OMBlock>()->isSolid(true));
    registerBlock(OMIdentifier("minecraft:spore_blossom"), std::make_shared<OMBlock>()->isSolid(false));
    registerBlock(OMIdentifier("minecraft:spruce_leaves"),
                  std::make_shared<OMBlock>()->isSolid(false)->setColorSource(BiomeFoliage));
    registerBlock(OMIdentifier("minecraft:spruce_log"), std::make_shared<OMBlock>()->isSolid(true));
    registerBlock(OMIdentifier("minecraft:sweet_berry_bush"), std::make_shared<OMBlock>()->isSolid(false));
    registerBlock(OMIdentifier("minecraft:tuff"), std::make_shared<OMBlock>()->isSolid(true));
    registerBlock(OMIdentifier("minecraft:vine"),
                  std::make_shared<OMBlock>()->isSolid(false)->setColorSource(BiomeFoliage));
}
} // namespace openminecraft::shell::data::block
