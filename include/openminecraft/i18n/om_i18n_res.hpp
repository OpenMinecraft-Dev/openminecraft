#ifndef OM_I18N_RES_HPP
#define OM_I18N_RES_HPP

#include <string>
#include <vector>

namespace openminecraft::i18n::res
{
struct LangInfo
{
    std::string modName;
    std::vector<std::string> locales;
};
void switchResourceRoot(std::string resRoot);
void registerModule(std::string name);
void removeModule(std::string name);
void load();
} // namespace openminecraft::i18n::res

#endif