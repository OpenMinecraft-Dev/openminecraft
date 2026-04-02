#ifndef OM_JFIF_IDCT_HPP
#define OM_JFIF_IDCT_HPP

#include <algorithm>
#include <array>
#include <cmath>
#include <iostream>

namespace openminecraft::specs::jfif
{
constexpr float PI_CONSTANT = 3.14159265358979323846;
extern double idctMap[8][8];
extern bool idctInited;
void idctInit();
static void idct_1d(const double in[8], double out[8])
{
    if (!idctInited)
    {
        idctInit();
    }
    const double sqrt2 = 1.4142135623730951;
    for (int i = 0; i < 8; ++i)
    {
        double sum = 0.0;
        for (int k = 0; k < 8; ++k)
        {
            double c = (k == 0) ? 1.0 / sqrt2 : 1.0;
            sum += c * in[k] * idctMap[i][k];
        }
        out[i] = sum;
    }
}

static void jpeg_idct(const std::array<int, 64> &input, std::array<int, 64> &output)
{
    double F[8][8];
    for (int i = 0; i < 8; ++i)
    {
        for (int j = 0; j < 8; ++j)
        {
            F[i][j] = static_cast<double>(input[i * 8 + j]);
        }
    }

    double M[8][8];
    for (int v = 0; v < 8; ++v)
    {
        double col[8];
        for (int u = 0; u < 8; ++u)
        {
            col[u] = F[u][v];
        }
        double res[8];
        idct_1d(col, res);
        for (int x = 0; x < 8; ++x)
        {
            M[x][v] = res[x];
        }
    }

    double f[8][8];
    for (int x = 0; x < 8; ++x)
    {
        double row[8];
        for (int v = 0; v < 8; ++v)
        {
            row[v] = M[x][v];
        }
        double res[8];
        idct_1d(row, res);
        for (int y = 0; y < 8; ++y)
        {
            f[x][y] = res[y];
        }
    }

    const double scale = 0.25;
    for (int i = 0; i < 8; ++i)
    {
        for (int j = 0; j < 8; ++j)
        {
            double val = f[i][j] * scale;
            val += 128.0;
            int ival = static_cast<int>(std::round(val));
            ival = std::clamp(ival, 0, 255);
            output[i * 8 + j] = ival;
        }
    }
}
}; // namespace openminecraft::specs::jfif

#endif
