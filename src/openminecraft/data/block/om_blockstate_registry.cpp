#include "openminecraft-shell/data/block/om_blockstate_registry.hpp"
#include "fmt/format.h"
#include "openminecraft-shell/data/block/om_block_registry.hpp"
#include "openminecraft-shell/data/om_identifier.hpp"
#include "openminecraft/log/om_log_common.hpp"

namespace openminecraft::shell::data::block
{
openminecraft::world::OMWorldRegistry<OMIdentifier, OMBlockStateCombined> blockStateRegistry;
static openminecraft::log::OMLogger logger("BlockState Registry");

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
            logger.debug("registering {}[{}]", l.first.path, st.tosimple());
            blockStateRegistry.registerItem(
                OMIdentifier(l.first.namesp, fmt::format("{}[{}]", l.first.path, st.tosimple())), c);
        }
    }
}
} // namespace openminecraft::shell::data::block