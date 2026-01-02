#include "openminecraft/fontproc/om_font.hpp"
#include "freetype/ftoutln.h"
#include "harfbuzz/hb.h"
#include "openminecraft/fontproc/om_font_outline.hpp"

#include "openminecraft/io/om_io_utils.hpp"

#include <fstream>
#include <glm/ext/matrix_transform.hpp>

namespace openminecraft::fontproc
{
long width, height;

OMFont::OMFont(std::istream &istr) : logger("OMFont", this)
{
    auto temp = io::readOnce(&istr);

    auto blob = hb_blob_create(reinterpret_cast<const char *>(temp.data()), temp.size(), HB_MEMORY_MODE_READONLY,
                               nullptr, nullptr);
    hbFace = hb_face_create(blob, 0);
    hbFont = hb_font_create(static_cast<hb_face_t *>(hbFace));
    hb_blob_destroy(blob);

    FT_Init_FreeType(&ftLibrary);
    FT_New_Memory_Face(ftLibrary, temp.data(), temp.size(), 0, &ftFace);
    FT_Set_Pixel_Sizes(ftFace, 0, 12);
}

void OMFont::parseChar(int charcode)
{
    char name[128];
    auto idx = FT_Get_Char_Index(ftFace, charcode);
    FT_Get_Glyph_Name(ftFace, idx, name, 128);
    FT_Load_Glyph(ftFace, idx, 0x00);
    width = std::max(ftFace->glyph->metrics.width, ftFace->glyph->metrics.horiAdvance);
    height = std::max(ftFace->glyph->metrics.height, ftFace->glyph->metrics.vertAdvance);

    auto otline = new OMFontOutline;
    FT_Outline_Funcs functest = {
        [](const FT_Vector *to, void *user) {
            static_cast<OMFontOutline *>(user)->moveTo(to);
            return 0;
        },
        [](const FT_Vector *to, void *user) {
            static_cast<OMFontOutline *>(user)->lineTo(to);
            return 0;
        },
        [](const FT_Vector *control, const FT_Vector *to, void *user) {
            static_cast<OMFontOutline *>(user)->conicTo(to, control);
            return 0;
        },
        [](const FT_Vector *control1, const FT_Vector *control2, const FT_Vector *to, void *user) {
            static_cast<OMFontOutline *>(user)->cubicTo(to, control1, control2);
            return 0;
        },
        0,
        0};
    FT_Outline_Decompose(&ftFace->glyph->outline, &functest, otline);

    auto stt = std::ofstream("out.csv");
    logger.info("{} x {}", width, height);

    std::vector<glm::vec2> points;
    glm::vec2 current = {};
    for (auto &t : otline->operations)
    {
        t.target /= glm::vec2{width, height};
        t.control1 /= glm::vec2{width, height};
        t.control2 /= glm::vec2{width, height};

#define prec 256

        switch (t.type)
        {
        case Move:
            current = t.target;
            break;
        case Line:
            points.push_back(current);
            current = t.target;
            points.push_back(current);
            break;
        case Conic:
            for (int i = 0; i <= prec; i++)
            {
                auto add = static_cast<float>(i) / static_cast<float>(prec);
                auto a = current * glm::vec2{(1 - add) * (1 - add)} +
                         t.control1 * glm::vec2{2 * add * (1 - add)} +
                         t.target * glm::vec2{add * add};
                points.push_back(a);
            }
            current = t.target;
            break;
        case Cubic:
            for (int i = 0; i <= prec; i++)
            {
                auto add = static_cast<float>(i) / static_cast<float>(prec);
                auto a = current * glm::vec2{(1 - add) * (1 - add) * (1 - add)} +
                         t.control1 * glm::vec2{3 * add * (1 - add) * (1 - add)} +
                         t.control2 * glm::vec2{3 * add * add * (1 - add)} +
                         t.target * glm::vec2{add * add * add};
                points.push_back(a);
            }
            current = t.target;
            break;
        }
    }
    for (auto &p : points)
    {
        auto fmt = fmt::format("{},{}\n", p.x, p.y);
        stt.write(fmt.c_str(), fmt.size());
    }
    stt.close();

    delete otline;
    logger.info("glyph name: {}", name);
}

OMFont::~OMFont()
{
    hb_font_destroy(static_cast<hb_font_t *>(hbFont));
    hb_face_destroy(static_cast<hb_face_t *>(hbFace));
    FT_Done_Face(ftFace);
    FT_Done_FreeType(ftLibrary);
}

} // namespace openminecraft::fontproc