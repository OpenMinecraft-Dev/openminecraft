#ifndef OM_JFIF_IDCT_HPP
#define OM_JFIF_IDCT_HPP

#include <array>

namespace openminecraft::specs::jfif
{
constexpr float PI_CONSTANT = 3.14159265358979323846;
extern std::array<std::array<double, 8>, 8> idctMap;
extern bool idctInited;
void idctInit();
}; // namespace openminecraft::specs::jfif

#endif
