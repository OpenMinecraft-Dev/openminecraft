#include "openminecraft/i18n/om_i18n_res.hpp"
#include "nlohmann/detail/exceptions.hpp"
#include "nlohmann/json.hpp"
#include "nlohmann/json_fwd.hpp"
#include "openminecraft/log/om_log_common.hpp"
#include "openminecraft/vfs/om_vfs_base.hpp"
#include <algorithm>
#include <iterator>

namespace openminecraft::i18n::res
{
std::string base;
std::vector<std::string> modNames;
std::vector<LangInfo> records;
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
void load()
{
    records.clear();
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
        }
        catch (nlohmann::detail::type_error e)
        {
            logger.error("parsing exception: {}", e.what());
        }
    }
}
} // namespace openminecraft::i18n::res