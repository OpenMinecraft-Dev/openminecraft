#ifndef OM_I18N_RES_HPP
#define OM_I18N_RES_HPP

#include <string>

namespace openminecraft::i18n::res
{
void switchResourceRoot(std::string resRoot);
void registerModule(std::string name);
void load();
} // namespace openminecraft::i18n::res

#endif