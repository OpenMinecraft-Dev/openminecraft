#include "openminecraft/fontproc/om_font.hpp"
#include "glm/glm.hpp"
#include "harfbuzz/hb.h"
#include "openminecraft/fontproc/om_font_outline.hpp"

#include "openminecraft/fontproc/om_font_triangle_list.hpp"
#include "openminecraft/io/om_io_utils.hpp"
#include "openminecraft/util/om_util_ticker.hpp"
#include <chrono>
#include <fstream>
#include <memory>
#include <thread>
#include <unordered_map>
#include <vector>

namespace openminecraft::fontproc
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

std::shared_ptr<OMTriangleList> OMFont::buildBasicPolygon(int charcode)
{
    util::OMTicker<std::string> ticker;
    ticker.tickStart();
    if (!drawfuncs)
    {
        drawfuncs = hb_draw_funcs_create();

        hb_draw_funcs_set_move_to_func(drawfuncs, acceptOutlineMoveTo, nullptr, nullptr);
        hb_draw_funcs_set_line_to_func(drawfuncs, acceptOutlineLineTo, nullptr, nullptr);
        hb_draw_funcs_set_quadratic_to_func(drawfuncs, acceptOutlineQuadraticTo, nullptr, nullptr);
        hb_draw_funcs_set_cubic_to_func(drawfuncs, acceptOutlineCubicTo, nullptr, nullptr);
        hb_draw_funcs_set_close_path_func(drawfuncs, acceptOutlineClosePath, nullptr, nullptr);

        ticker.recordEvent("glyph_draw_funcs");
    }

    auto font = static_cast<hb_font_t *>(hbFont);

    hb_codepoint_t gly;
    hb_font_get_nominal_glyph(font, charcode, &gly);

    OMFontOutline outline;

    hb_font_draw_glyph(font, gly, drawfuncs, &outline);
    ticker.recordEvent("glyph_hb_font_draw_glyph");
    int xsc, ysc;
    hb_font_get_scale(font, &xsc, &ysc);

    auto rawpoly = outline.buildPolygons(8, xsc, ysc);
    std::sort(rawpoly.begin(), rawpoly.end(), [](std::shared_ptr<OMFontPolygon> p1, std::shared_ptr<OMFontPolygon> p2) {
        return p1->area() > p2->area();
    });
    ticker.recordEvent("glyph_polygons");

    std::unordered_map<int, int> parents;
    for (int i = 0; i < rawpoly.size(); i++)
    {
        // gino: -1 for virtual root
        int parent = -1;
        for (int j = 0; j < i; j++)
        {
            if (rawpoly[j]->isPolyInside(rawpoly[i]))
            {
                parent = j;
            }
        }

        parents[i] = parent;
    }
    ticker.recordEvent("glyph_polygon_tree");

    std::vector<int> filledPoly;
    for (int pi = 0; pi < rawpoly.size(); pi++)
    {
        auto currentIdx = pi;
        int depth = 0;
        while (currentIdx != -1)
        {
            currentIdx = parents[currentIdx];
            depth++;
        }

        // gino: the depth is even, means that this polygon need to be rendered
        if (depth & 0x1)
        {
            filledPoly.push_back(pi);
        }
    }
    ticker.recordEvent("glyph_determine_poly_fill");

    std::vector<std::shared_ptr<OMTriangleList>> listbase;
    for (auto polyid : filledPoly)
    {
        std::vector<std::shared_ptr<OMFontPolygon>> polys;
        for (auto sid : parents)
        {
            if (sid.second == polyid)
            {
                polys.push_back(rawpoly[sid.first]);
            }
        }

        listbase.push_back(std::make_shared<OMTriangleList>(rawpoly[polyid], polys));
    }
    ticker.recordEvent("glyph_triangulation");

    logger.info("Glyph build process: ");
    for (auto &pp : ticker.fetchEvents<std::chrono::microseconds>())
    {
        logger.info("{}: ~{} us", pp.first, pp.second);
    }

    return std::make_shared<OMTriangleList>(listbase);
}

std::shared_ptr<OMFontGlyph> OMFont::buildGlyph(int charcode)
{
    auto ots = buildBasicPolygon(charcode);

    auto font = static_cast<hb_font_t *>(hbFont);

    int xsc, ysc;
    hb_font_get_scale(font, &xsc, &ysc);

    hb_codepoint_t gly;
    hb_font_get_nominal_glyph(font, charcode, &gly);

    hb_glyph_extents_t extents;
    hb_font_get_glyph_extents(font, gly, &extents);

    glm::vec4 siz = {
        static_cast<float>(extents.x_bearing) / xsc, static_cast<float>(extents.x_bearing + extents.width) / xsc,
        static_cast<float>(extents.y_bearing + extents.height) / ysc, static_cast<float>(extents.y_bearing) / ysc};
    return std::make_shared<OMFontGlyph>(ots, siz);
}

OMFont::~OMFont()
{
    hb_font_destroy(static_cast<hb_font_t *>(hbFont));
    hb_face_destroy(static_cast<hb_face_t *>(hbFace));
}

} // namespace openminecraft::fontproc
