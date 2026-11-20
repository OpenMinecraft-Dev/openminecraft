#ifndef OM_I18N_LOCALE_HPP
#define OM_I18N_LOCALE_HPP

#include <string>
#include <vector>

namespace openminecraft::i18n::locale
{
auto defaultLocale() -> std::string;
auto available() -> std::vector<std::string>;
} // namespace openminecraft::i18n::locale

#endif
