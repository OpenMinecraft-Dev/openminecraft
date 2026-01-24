#include "openminecraft/i18n/om_i18n_res.hpp"
#include "openminecraft/i18n/om_i18n_locale.hpp"
#include "openminecraft/io/json/om_io_ast_builder_json.hpp"
#include "openminecraft/io/json/om_io_tokeniter_json.hpp"
#include "openminecraft/io/om_io_tokeniter_exception.hpp"
#include "openminecraft/log/om_log_common.hpp"
#include "openminecraft/vfs/om_vfs_base.hpp"
#include <SDL3/SDL_locale.h>
#include <algorithm>
#include <iterator>
#include <unordered_map>

using namespace openminecraft::io;

namespace openminecraft::i18n::res
{
std::vector<std::string> base;
std::vector<std::string> modNames;
std::vector<LangInfo> records;
std::string locale;
std::unordered_map<std::string, std::unordered_map<std::string, std::string>> translates;
log::OMLogger logger = log::OMLogger("i18n");
void pushResourceRoot(std::string resRoot)
{
    base.push_back(resRoot);
}
void registerModule(std::string name)
{
    modNames.push_back(name);
}
void removeModule(std::string name)
{
    auto d = std::find(modNames.begin(), modNames.end(), name);
    if (d != modNames.end())
    {
        modNames.erase(modNames.begin() + std::distance(modNames.begin(), d));
    }
}
void updateLocale(std::string loc)
{
    locale = loc;
}
void load()
{
    locale = locale::defaultLocale();

    records.clear();
    translates.clear();
    for (auto mod : modNames)
    {
        try
        {
            for (auto rt : base)
            {
                auto p = fmt::format("{}/{}/lang/lang.json", rt, mod);

                auto ff = vfs::fsfetch(p);
                if (!ff || !ff->good())
                {
                    logger.warn("Bad module language list file: {}", p);
                }
                json::OMJsonAstBuilder bld(std::make_shared<json::OMJsonTokenIter>(ff));
                auto ll = bld.build();

                std::vector<std::string> loc;
                for (auto &a : ll->getMap()["available"]->getArray())
                {
                    loc.push_back(a->getString());
                }
                records.push_back({mod, loc});

                for (auto l : loc)
                {
                    if (translates.count(l) == 0)
                    {
                        translates[l] = std::unordered_map<std::string, std::string>();
                    }

                    auto pr = fmt::format("{}/{}/lang/{}.json", rt, mod, l);
                    auto fr = vfs::fsfetch(pr);
                    if (!fr || !fr->good())
                    {
                        logger.warn("Bad module language file: {}", pr);
                        continue;
                    }

                    json::OMJsonAstBuilder bld2(std::make_shared<json::OMJsonTokenIter>(fr));
                    auto ll2 = bld2.build();

                    for (auto &p : ll2->getMap())
                    {
                        translates[l][p.first] = p.second->getString();
                    }
                }
            }
        }
        catch (std::logic_error e)
        {
            logger.error("parsing exception: {}", e.what());
        }
        catch (OMTokenIterException e)
        {
            logger.error("tokenizer exception: {}", e.what());
        }
    }
}

std::string translate(std::string key)
{
    auto data = translates[locale][key];
    if (!data.empty())
    {
        return data;
    }
    auto fallback = translates[DefaultLocale][key];
    if (!fallback.empty())
    {
        return fallback;
    }
    return key;
}
} // namespace openminecraft::i18n::res
