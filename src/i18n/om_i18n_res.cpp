#include "openminecraft/i18n/om_i18n_res.hpp"
#include "nlohmann/detail/exceptions.hpp"
#include "nlohmann/json.hpp"
#include "nlohmann/json_fwd.hpp"
#include "openminecraft/log/om_log_common.hpp"
#include "openminecraft/vfs/om_vfs_base.hpp"
#include <SDL3/SDL_locale.h>
#include <algorithm>
#include <cctype>
#include <iterator>

namespace openminecraft::i18n::res
{
std::string base;
std::vector<std::string> modNames;
std::vector<LangInfo> records;
std::string locale;
std::map<std::string, std::map<std::string, std::string>> translates;
log::OMLogger logger = log::OMLogger("i18n");
void switchResourceRoot(std::string resRoot)
{
    base = resRoot;
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
    int c;
    SDL_Locale **loc = SDL_GetPreferredLocales(&c);
    if (c > 0)
    {
        std::string c = loc[0]->country;
        std::transform(c.begin(), c.end(), c.begin(), ::tolower);
        locale = fmt::format("{}_{}", loc[0]->language, c);
    }

    records.clear();
    translates.clear();
    nlohmann::json data;
    for (auto mod : modNames)
    {
        try
        {
            auto p = fmt::format("{}/{}/lang/lang.json", base, mod);
            auto f = vfs::fsfetch(p);
            if (!f || !f->good())
            {
                logger.info("Bad module language list file: {}", p);
                continue;
            }

            *f >> data;
            std::vector<std::string> loc;
            for (auto a : data["available"])
            {
                loc.push_back((std::string)a);
            }
            records.push_back({mod, loc});

            for (auto l : loc)
            {
                if (translates.count(l) == 0)
                {
                    translates[l] = std::map<std::string, std::string>();
                }

                auto pr = fmt::format("{}/{}/lang/{}.json", base, mod, l);
                auto fr = vfs::fsfetch(pr);
                if (!fr || !fr->good())
                {
                    logger.info("Bad module language file: {}", pr);
                    continue;
                }

                *fr >> data;
                for (auto m : data.items())
                {
                    translates[l][(std::string)m.key()] = (std::string)m.value();
                }
            }
        }
        catch (nlohmann::detail::type_error e)
        {
            logger.error("parsing exception: {}", e.what());
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
    return translates[DefaultLocale][key];
}
} // namespace openminecraft::i18n::res