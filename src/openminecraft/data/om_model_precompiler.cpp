#include <iostream>
#include <utility>

#include "openminecraft-shell/data/om_model_precompiler.hpp"
#include "fmt/format.h"
#include "openminecraft-shell/data/om_identifier.hpp"
#include "openminecraft/io/json/om_io_ast_builder_json.hpp"
#include "openminecraft/vfs/om_vfs_base.hpp"

using namespace openminecraft;
using namespace openminecraft::io;

namespace openminecraftshell::data
{
OMModelPrecompiler::OMModelPrecompiler(std::string root) : root(std::move(root)), logger("OMModelPrecompiler", this)
{
}

auto OMModelPrecompiler::precompile(OMIdentifier name, bool subsitute)
    -> std::shared_ptr<openminecraft::io::json::OMJsonNode>
{
    auto ff = vfs::fsfetch(fmt::format("{}/{}/models/{}.json", root, name.namesp, name.path));
    json::OMJsonAstBuilder bld(std::make_shared<json::OMJsonTokenIter>(ff));
    auto ll = bld.build();

    if (ll->getMap().count("parent"))
    {
        auto sub = precompile(OMIdentifier(ll->getMap()["parent"]->getString()), false);
        ll->getMap().erase("parent");
        ll->merge(sub);
    }

    if (subsitute)
    {
        auto &tex = ll->getMap()["textures"]->getMap();
        for (auto &pp : tex)
        {
            if (pp.second->getString()[0] == '#' && tex.count(pp.second->getString().substr(1)))
            {
                pp.second = tex[pp.second->getString().substr(1)];
            }
        }

        for (auto &elem : ll->getMap()["elements"]->getArray())
        {
            for (auto &fce : elem->getMap()["faces"]->getMap())
            {
                auto text = fce.second->getMap()["texture"]->getString();

                if (text[0] == '#' && tex.count(text.substr(1)))
                {
                    fce.second->getMap()["texture"] = tex[text.substr(1)];
                }
            }
        }
        return ll->getMap()["elements"];
    }
    else
    {
        return ll;
    }
}
} // namespace openminecraftshell::data