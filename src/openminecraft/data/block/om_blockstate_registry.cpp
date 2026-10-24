#include "openminecraft-shell/data/block/om_blockstate_registry.hpp"
#include "openminecraft-shell/data/block/om_block_registry.hpp"
#include "openminecraft-shell/data/om_identifier.hpp"

namespace openminecraft::shell::data::block
{
openminecraft::world::OMWorldRegistry<OMBlockStateCombined, OMBlockStateCombined> &blockStateRegistry =
    *new openminecraft::world::OMWorldRegistry<OMBlockStateCombined, OMBlockStateCombined>();

void registerBlockStates()
{
    for (auto &l : blockRegistry.nameToId)
    {
        std::vector<OMBlockState> states = {};
        states.emplace_back("");

        for (const auto &property : blockRegistry.getRegistry(l.first)->properties)
        {
            auto oldStates = states;
            std::vector<OMBlockState> newStates;
            for (const auto &v : property.second)
            {
                for (const auto &os : oldStates)
                {
                    auto raw = os.properties;
                    raw[property.first] = v;
                    newStates.emplace_back(raw);
                }
            }

            states = newStates;
        }
        for (auto &st : states)
        {
            auto c = OMBlockStateCombined{l.first, st};
            blockStateRegistry.registerItem(c, c);
        }
    }
}
} // namespace openminecraft::shell::data::block