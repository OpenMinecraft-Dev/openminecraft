#include "openminecraft/i18n/om_i18n_locale.hpp"
#include <SDL3/SDL_locale.h>
#include <algorithm>
#include <cctype>
#include <fmt/base.h>
#include <fmt/format.h>
#include <vector>

namespace openminecraft::i18n::locale
{
std::string defaultLocale()
{
    int c;
    SDL_Locale **d = SDL_GetPreferredLocales(&c);
    if (c == 0)
    {
        return "en_us";
    }
    else
    {
        auto dd = fmt::format("{}_{}", d[0]->language, d[0]->country);
        std::transform(dd.begin(), dd.end(), dd.begin(), ::tolower);
        return dd;
    }
}

std::vector<std::string> available()
{
    return {"zh_cn", "en_us", "ja_jp"};
}
} // namespace openminecraft::i18n::locale