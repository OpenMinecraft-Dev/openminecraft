#include "openminecraft/specs/jfif/om_jfif_idct.hpp"
#include <cmath>

namespace openminecraft::specs::jfif
{
double idctMap[8][8];
bool idctInited = false;

void idctInit()
{
    for (int i = 0; i < 8; i++)
    {
        for (int k = 0; k < 8; k++)
        {
            idctMap[i][k] = std::cos((2 * i + 1) * k * PI_CONSTANT / 16.0);
        }
    }
    idctInited = true;
}
} // namespace openminecraft::specs::jfif
