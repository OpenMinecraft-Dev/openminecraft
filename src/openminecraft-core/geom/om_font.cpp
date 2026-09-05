#include "openminecraft/geom/om_font.hpp"
#include "glm/ext/vector_float2.hpp"
#include "glm/ext/vector_float4.hpp"
#include "harfbuzz/hb.h"
#include "openminecraft/geom/om_font_outline.hpp"
#include "openminecraft/io/om_io_utils.hpp"

#include <iostream>
#include <vector>

namespace openminecraft::geom
{
OMFont::OMFont(std::istream &istr) : logger("OMFont", this)
{
    auto temp = io::readOnce(&istr);

    auto blob = hb_blob_create(reinterpret_cast<const char *>(temp.data()), temp.size(), HB_MEMORY_MODE_DUPLICATE,
                               nullptr, nullptr);
    hbFace = hb_face_create(blob, 0);
    hbFont = hb_font_create(static_cast<hb_face_t *>(hbFace));
    hb_blob_destroy(blob);
}

static void acceptOutlineMoveTo(hb_draw_funcs_t *, void *drawdata, hb_draw_state_t *state, float x, float y,
                                void *user_data)
{
    static_cast<OMFontOutline *>(drawdata)->moveTo({x, y});
}

static void acceptOutlineLineTo(hb_draw_funcs_t *, void *drawdata, hb_draw_state_t *state, float x, float y,
                                void *user_data)
{
    static_cast<OMFontOutline *>(drawdata)->lineTo({x, y});
}

static void acceptOutlineQuadraticTo(hb_draw_funcs_t *, void *drawdata, hb_draw_state_t *state, float cx, float cy,
                                     float x, float y, void *user_data)
{
    static_cast<OMFontOutline *>(drawdata)->quadraticTo({x, y}, {cx, cy});
}

static void acceptOutlineCubicTo(hb_draw_funcs_t *, void *drawdata, hb_draw_state_t *state, float cx1, float cy1,
                                 float cx2, float cy2, float x, float y, void *user_data)
{
    static_cast<OMFontOutline *>(drawdata)->cubicTo({x, y}, {cx1, cy1}, {cx2, cy2});
}

static void acceptOutlineClosePath(hb_draw_funcs_t *, void *drawdata, hb_draw_state_t *state, void *user_data)
{
    static_cast<OMFontOutline *>(drawdata)->closePath();
}

hb_draw_funcs_t *drawfuncs = nullptr;

auto OMFont::scale() -> glm::vec2
{
    auto font = static_cast<hb_font_t *>(hbFont);

    int xsc, ysc;
    hb_font_get_scale(font, &xsc, &ysc);

    return {xsc, ysc};
}
auto OMFont::buildOutline(int charcode, bool uni) -> OMFontOutline
{
    if (!drawfuncs)
    {
        drawfuncs = hb_draw_funcs_create();

        hb_draw_funcs_set_move_to_func(drawfuncs, acceptOutlineMoveTo, nullptr, nullptr);
        hb_draw_funcs_set_line_to_func(drawfuncs, acceptOutlineLineTo, nullptr, nullptr);
        hb_draw_funcs_set_quadratic_to_func(drawfuncs, acceptOutlineQuadraticTo, nullptr, nullptr);
        hb_draw_funcs_set_cubic_to_func(drawfuncs, acceptOutlineCubicTo, nullptr, nullptr);
        hb_draw_funcs_set_close_path_func(drawfuncs, acceptOutlineClosePath, nullptr, nullptr);
    }

    auto font = static_cast<hb_font_t *>(hbFont);

    hb_codepoint_t gly = charcode;
    if (uni)
    {
        hb_font_get_nominal_glyph(font, charcode, &gly);
    }

    OMFontOutline outline;

    hb_font_draw_glyph(font, gly, drawfuncs, &outline);

    return outline;
}

auto OMFont::fetchBox(int charcode, bool uni) -> glm::vec4
{
    auto font = static_cast<hb_font_t *>(hbFont);
    auto s = scale();

    hb_codepoint_t gly = charcode;
    if (uni)
    {
        hb_font_get_nominal_glyph(font, charcode, &gly);
    }
    hb_glyph_extents_t extents;
    hb_font_get_glyph_extents(font, gly, &extents);

    return {static_cast<float>(extents.x_bearing) / s.x, static_cast<float>(extents.x_bearing + extents.width) / s.x,
            static_cast<float>(extents.y_bearing + extents.height) / s.y, static_cast<float>(extents.y_bearing) / s.y};
}

auto OMFont::shape(std::string s, bool &isRTL) -> std::vector<OMFontShapeResult>
{
    auto buf = hb_buffer_create();
    hb_buffer_add_utf8(buf, s.c_str(), -1, 0, -1);
    hb_buffer_guess_segment_properties(buf);
    isRTL = hb_buffer_get_direction(buf) == HB_DIRECTION_RTL;

    hb_shape(static_cast<hb_font_t *>(hbFont), buf, nullptr, 0);

    uint32_t glyphs;
    auto glyphInfo = hb_buffer_get_glyph_infos(buf, &glyphs);
    auto glyphPos = hb_buffer_get_glyph_positions(buf, &glyphs);
    std::vector<OMFontShapeResult> result = {};

    auto ss = scale();

    for (int i = 0; i < glyphs; ++i)
    {
        result.emplace_back(OMFontShapeResult{glyphInfo[i].codepoint, glyphInfo[i].cluster, glyphPos[i].x_offset / ss.x,
                                              glyphPos[i].y_offset / ss.y, glyphPos[i].x_advance / ss.x,
                                              glyphPos[i].y_advance / ss.y, this});
    }

    hb_buffer_destroy(buf);

    return result;
}

auto OMFont::metrics(bool rtl) -> glm::vec4
{
    auto font = static_cast<hb_font_t *>(hbFont);
    auto s = scale();
    hb_font_extents_t extents;
    hb_font_get_extents_for_direction(font, rtl ? HB_DIRECTION_RTL : HB_DIRECTION_LTR, &extents);

    return {float(extents.ascender) / s.y, float(extents.descender) / s.y, float(extents.line_gap) / s.y, 0.0f};
}

OMFont::~OMFont()
{
    hb_font_destroy(static_cast<hb_font_t *>(hbFont));
    hb_face_destroy(static_cast<hb_face_t *>(hbFace));
}

} // namespace openminecraft::geom
