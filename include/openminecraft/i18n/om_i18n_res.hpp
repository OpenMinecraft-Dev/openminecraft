#ifndef OM_I18N_RES_HPP
#define OM_I18N_RES_HPP

#include "fmt/format.h"
#include <string>
#include <vector>

#define DefaultLocale "en_us"
namespace openminecraft::i18n::res
{
struct LangInfo
{
    std::string modName;
    std::vector<std::string> locales;
};
void pushResourceRoot(std::string resRoot);
void registerModule(std::string name);
void removeModule(std::string name);
void load();
void updateLocale(std::string loc);
std::string translate(std::string key);
template <typename... T> std::string translate(std::string key, T... args)
{
    return translate(fmt::format(key, args...));
}
} // namespace openminecraft::i18n::res

#endif