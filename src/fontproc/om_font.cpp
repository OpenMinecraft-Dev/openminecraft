#include "openminecraft/fontproc/om_font.hpp"
#include "harfbuzz/hb.h"
#include "openminecraft/fontproc/om_font_outline.hpp"

#include "openminecraft/io/om_io_utils.hpp"

#include <fstream>
#include <glm/ext/matrix_transform.hpp>

namespace openminecraft::fontproc
{

OMFont::OMFont(std::istream &istr) : logger("OMFont", this)
{
    auto temp = io::readOnce(&istr);

    auto blob = hb_blob_create(reinterpret_cast<const char *>(temp.data()), temp.size(), HB_MEMORY_MODE_READONLY,
                               nullptr, nullptr);
    hbFace = hb_face_create(blob, 0);
    hbFont = hb_font_create(static_cast<hb_face_t *>(hbFace));
    hb_blob_destroy(blob);
}

static void acceptOutlineMoveTo(hb_draw_funcs_t *, void *drawdata, hb_draw_state_t *state, float x, float y, void *user_data)
{
    static_cast<OMFontOutline *>(drawdata)->moveTo({x, y});
}

static void acceptOutlineLineTo(hb_draw_funcs_t *, void *drawdata, hb_draw_state_t *state, float x, float y, void *user_data)
{
    static_cast<OMFontOutline *>(drawdata)->lineTo({x, y});
}

static void acceptOutlineQuadraticTo(hb_draw_funcs_t *, void *drawdata, hb_draw_state_t *state, float cx, float cy, float x, float y, void *user_data)
{
    static_cast<OMFontOutline *>(drawdata)->quadraticTo({x, y}, {cx, cy});
}

static void acceptOutlineCubicTo(hb_draw_funcs_t *, void *drawdata, hb_draw_state_t *state, float cx1, float cy1, float cx2, float cy2, float x, float y, void *user_data)
{
    static_cast<OMFontOutline *>(drawdata)->cubicTo({x, y}, {cx1, cy1}, {cx2, cy2});
}

static void acceptOutlineClosePath(hb_draw_funcs_t *, void *drawdata, hb_draw_state_t *state, void *user_data)
{
    static_cast<OMFontOutline *>(drawdata)->closePath();
}

void OMFont::parseChar(int charcode)
{
    auto font = static_cast<hb_font_t *>(hbFont);
    hb_font_set_scale(font, 1, 1);

    hb_codepoint_t gly;
    hb_font_get_nominal_glyph(font, charcode, &gly);

    OMFontOutline outline;
    auto funcs = hb_draw_funcs_create();

    hb_draw_funcs_set_move_to_func(funcs, acceptOutlineMoveTo, this, nullptr);
    hb_draw_funcs_set_line_to_func(funcs, acceptOutlineLineTo, this, nullptr);
    hb_draw_funcs_set_quadratic_to_func(funcs, acceptOutlineQuadraticTo, this, nullptr);
    hb_draw_funcs_set_cubic_to_func(funcs, acceptOutlineCubicTo, this, nullptr);
    hb_draw_funcs_set_close_path_func(funcs, acceptOutlineClosePath, this, nullptr);

    hb_font_draw_glyph(font, gly, funcs, &outline);
    hb_draw_funcs_destroy(funcs);

    int xsc, ysc;
    hb_font_get_scale(font, &xsc, &ysc);

    auto ll = outline.buildPolygons(2, xsc, ysc);
    logger.info("{}", ll.size());
    for (auto &p : ll)
    {
        logger.info("area: {}", p->area());
    }
}

OMFont::~OMFont()
{
    hb_font_destroy(static_cast<hb_font_t *>(hbFont));
    hb_face_destroy(static_cast<hb_face_t *>(hbFace));
}

} // namespace openminecraft::fontproc