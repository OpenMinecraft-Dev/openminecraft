#include "openminecraft-shell/data/block/om_blockstate_resolver.hpp"
#include "openminecraft-shell/data/block/om_block.hpp"
#include "openminecraft-shell/data/block/om_block_registery.hpp"
#include "openminecraft-shell/data/block/om_blockstate.hpp"
#include "openminecraft-shell/data/om_identifier.hpp"
#include "openminecraft/io/json/om_io_ast_json.hpp"
#include "openminecraft/vfs/om_vfs_base.hpp"
#include "openminecraft/io/json/om_io_ast_builder_json.hpp"
#include <memory>
#include <vector>

using namespace openminecraft::io;

namespace openminecraftshell::data::block
{
auto OMBlockstateResolver::identFrom(std::shared_ptr<io::json::OMJsonNode> node) -> OMBlockModelIdentifier
{
    OMIdentifier ident(node->getMap()["model"]->getString());
    int xrot = node->getMap().count("x") ? node->getMap()["x"]->getNumber() : 0;
    int yrot = node->getMap().count("y") ? node->getMap()["y"]->getNumber() : 0;
    int zrot = node->getMap().count("z") ? node->getMap()["z"]->getNumber() : 0;
    bool uvlock = node->getMap().count("uvlock") ? node->getMap()["uvlock"]->getBoolean() : false;
    return {ident, xrot, yrot, zrot, uvlock};
}
void OMBlockstateResolver::resolveModel(OMIdentifier id, std::shared_ptr<io::json::OMJsonNode> node)
{
    auto i = identFrom(node);
    if (requiredModels[id].count(i))
    {
        return;
    }
    requiredModels[id][i] = compiler.loadModelPartWithArgs(i.location, i.xrot, i.yrot, i.zrot, i.uvlock);
}
void OMBlockstateResolver::resolve(OMIdentifier ident)
{
    auto &blk = blockRegistery.getRegistry(ident);
    states[ident].clear();
    requiredModels[ident].clear();

    auto ff = vfs::fsfetch(fmt::format("{}/{}/blockstates/{}.json", root, ident.namesp, ident.path));
    json::OMJsonAstBuilder bld(std::make_shared<json::OMJsonTokenIter>(ff));
    auto ll = bld.build();

    if (ll->getMap().count("variants"))
    {
        for (auto &l : ll->getMap()["variants"]->getMap())
        {
            if (l.second->type() == openminecraft::io::json::Object)
            {
                resolveModel(ident, l.second);
            }
            else
            {
                for (auto &lp : l.second->getArray())
                {
                    resolveModel(ident, lp);
                }
            }
        }
    }
    else if (ll->getMap().count("multipart"))
    {
        for (auto &l : ll->getMap()["multipart"]->getArray())
        {
            auto &ll = l->getMap()["apply"];
            if (ll->type() == openminecraft::io::json::Object)
            {
                resolveModel(ident, ll);
            }
            else
            {
                for (auto &lp : ll->getArray())
                {
                    resolveModel(ident, lp);
                }
            }
        }
    }
    resolverCache[ident] = ll;
}

static auto caseCheck(OMBlockState &bs, std::shared_ptr<json::OMJsonNode> node) -> bool
{
    if (!node)
        return true;

    if (node->type() != json::Object)
        return true;

    for (auto &st : node->getMap())
    {
        if (st.first == "AND")
        {
            if (st.second->type() == json::Array)
            {
                for (auto &p : st.second->getArray())
                    if (!caseCheck(bs, p))
                        return false;
            }
            else
            {
                if (!caseCheck(bs, st.second))
                    return false;
            }
        }
        else if (st.first == "OR")
        {
            bool any_match = false;
            if (st.second->type() == json::Array)
            {
                for (auto &p : st.second->getArray())
                    if (caseCheck(bs, p))
                    {
                        any_match = true;
                        break;
                    }
            }
            else
            {
                any_match = caseCheck(bs, st.second);
            }
            if (!any_match)
                return false;
        }
        else
        {
            if (!bs.properties.count(st.first))
                return false;

            std::string target;
            if (st.second->type() == openminecraft::io::json::String)
            {
                target = st.second->getString();
            }
            else if (st.second->type() == openminecraft::io::json::Primitive)
            {
                target = st.second->getBoolean() ? "true" : "false";
            }
            else if (st.second->type() == openminecraft::io::json::Number)
            {
                target = st.second->getNumber();
            }

            auto pos = target.find('|');
            if (pos != std::string::npos)
            {
                return target.find(bs.properties[st.first]) != std::string::npos;
            }
            else
            {
                if (bs.properties[st.first] != target)
                    return false;
            }
        }
    }
    return true;
}

auto OMBlockstateResolver::fetchModel(OMIdentifier ident, OMBlockState state, int variant) -> int
{
    if (states[ident].count(state))
    {
        auto &m = states[ident][state];
        return m[variant % m.size()];
    }
    else
    {
        return 0;
    }
}

void OMBlockstateResolver::buildModel(OMIdentifier ident, OMBlockState state)
{
    auto &blk = blockRegistery.getRegistry(ident);
    auto &st = state;

    if (states[ident].count(st))
    {
        return;
    }

    if (resolverCache[ident]->getMap().count("variants"))
    {
        for (auto &var : resolverCache[ident]->getMap()["variants"]->getMap())
        {
            auto bs = OMBlockState(var.first);
            for (auto &k : bs.properties)
            {
                if (st.properties[k.first] != k.second)
                {
                    goto next;
                }
            }
            {
                if (var.second->type() == openminecraft::io::json::Object)
                {
                    auto i = compiler.composeBlock({requiredModels[ident][identFrom(var.second)]}, blk.soild,
                                                   blk.translucent);
                    states[ident][st].emplace_back(i);
                    return;
                }
                else
                {
                    for (auto &lp : var.second->getArray())
                    {
                        auto i =
                            compiler.composeBlock({requiredModels[ident][identFrom(lp)]}, blk.soild, blk.translucent);
                        states[ident][st].emplace_back(i);
                    }
                    return;
                }
            }
        next:
            continue;
        }
    }
    else if (resolverCache[ident]->getMap().count("multipart"))
    {
        std::vector<int> ids = {};
        for (auto &l : resolverCache[ident]->getMap()["multipart"]->getArray())
        {
            if (!caseCheck(st, l->getMap().count("when") ? l->getMap()["when"] : nullptr))
            {
                continue;
            }

            auto &ll = l->getMap()["apply"];
            if (ll->type() == openminecraft::io::json::Object)
            {
                ids.emplace_back(requiredModels[ident][identFrom(ll)]);
            }
            else
            {
                for (auto &lp : ll->getArray())
                {
                    ids.emplace_back(requiredModels[ident][identFrom(lp)]);
                }
            }
        }

        auto i = compiler.composeBlock(ids, blk.soild, blk.translucent);
        states[ident][st].emplace_back(i);
        return;
    }
}
} // namespace openminecraftshell::data::block