#ifndef OM_I18N_LOCALE_HPP
#define OM_I18N_LOCALE_HPP

#include <string>
#include <vector>

namespace openminecraft::i18n::locale
{
std::string defaultLocale();
std::vector<std::string> available();
} // namespace openminecraft::i18n::locale

#endif