#ifndef OM_DEMIURGE_SRGB_HPP
#define OM_DEMIURGE_SRGB_HPP

#include "glm/glm.hpp"
#include <cstdint>
namespace openminecraft::renderer::common::demiurge
{
inline auto srgbToLinear(glm::vec4 c) -> glm::vec4
{
    auto s2l = [](float v) {
        if (v <= 0.04045f)
            return v / 12.92f;
        else
            return glm::pow((v + 0.055f) / 1.055f, 2.4f);
    };
    return {s2l(c.r), s2l(c.g), s2l(c.b), c.a};
}

inline auto genLinear(uint8_t r, uint8_t g, uint8_t b, uint8_t a) -> glm::vec4
{
    return srgbToLinear({r / 255.0f, g / 255.0f, b / 255.0f, a / 255.0f});
}

inline auto genLinear(uint32_t rgba) -> glm::vec4
{
    return genLinear((rgba >> 24) & 0xFF, (rgba >> 16) & 0xFF, (rgba >> 8) & 0xFF, rgba & 0xFF);
}
} // namespace openminecraft::renderer::common::demiurge

#endif
