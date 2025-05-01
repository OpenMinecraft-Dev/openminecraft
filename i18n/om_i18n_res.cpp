#include "openminecraft/i18n/om_i18n_res.hpp"
#include "nlohmann/json.hpp"
#include "nlohmann/json_fwd.hpp"
#include "openminecraft/log/om_log_common.hpp"
#include "openminecraft/vfs/om_vfs_base.hpp"
#include <vector>

namespace openminecraft::i18n::res
{
std::string base;
std::vector<std::string> modNames;
log::OMLogger logger = log::OMLogger("i18n");
void switchResourceRoot(std::string resRoot)
{
    base = resRoot;
}
void registerModule(std::string name)
{
    modNames.push_back(name);
}
void load()
{
    nlohmann::json data;
    for (auto mod : modNames)
    {
        auto p = fmt::format("{}/{}/lang/lang.json", base, mod);
        auto f = vfs::fsfetch(p);
        if (!f->good())
        {
            logger.info("Bad module language list file: {}", p);
            continue;
        }

        *f >> data;
        for (auto a : data["available"])
        {
            logger.info("{}", (std::string)a);
        }
    }
}
} // namespace openminecraft::i18n::res