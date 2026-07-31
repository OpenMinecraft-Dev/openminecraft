#include "openminecraft/fontproc/om_fontset.hpp"
#include "glm/ext/vector_float2.hpp"
#include "openminecraft/fontproc/om_font.hpp"
#include "openminecraft/fontproc/om_font_outline.hpp"
#include <vector>
#include <glm/glm.hpp>

namespace openminecraft::fontproc
{
auto OMFontSet::shape(std::string s) -> std::vector<OMFontSetShapeResult>
{
    std::vector<OMFontShapeResult> glyphs = {};
    int id = 0;
    for (auto f : fontList)
    {
        auto r = f->shape(s);

        if (glyphs.size() < r.size())
        {
            glyphs.resize(r.size());
        }

        for (int i = 0; i < r.size(); ++i)
        {
            if (glyphs[i].glyphId == 0x0 && r[i].glyphId != 0x0)
            {
                glyphs[i] = r[i];
                glyphs[i].fontId = id;
            }
        }
        ++id;
    }

    std::vector<OMFontSetShapeResult> result;
    glm::vec2 penpos(0.0f);

    float miny = 0.0f;
    for (auto &g : glyphs)
    {
        auto bbox = g.font->fetchBox(g.glyphId, false);
        float left = bbox.x;
        float right = bbox.y;
        float top = bbox.z;
        float bottom = bbox.w;

        float x = penpos.x + g.offsetx + left;
        float y = -(penpos.y + g.offsety + top);
        float w = right - left;
        float h = bottom - top;

        miny = std::min(miny, y);
        result.emplace_back(OMFontSetShapeResult{g.font, g.fontId, g.glyphId, glm::vec2(x, y), glm::vec2(w, h)});

        penpos += glm::vec2(g.advancex, g.advancey);
    }

    for (auto &r : result)
    {
        r.position.y -= miny;
    }

    return result;
}

static auto approximateCubicToQuadratic(glm::vec2 P0, glm::vec2 P1, glm::vec2 P2, glm::vec2 P3) -> glm::vec2
{
    glm::vec2 d1 = P1 - P0;
    glm::vec2 d2 = P3 - P2;
    float det = d1.x * d2.y - d1.y * d2.x;
    if (fabs(det) < 1e-6)
    {
        return (P1 + P2) * glm::vec2(0.5);
    }
    glm::vec2 diff = P3 - P0;
    float s = (diff.x * d2.y - diff.y * d2.x) / det;

    return P0 + s * d1;
}

auto OMFontSet::bound(std::string s) -> glm::vec2
{
    auto shaped = shape(s);
    if (shaped.empty())
        return glm::vec2(0.0f);

    glm::vec2 maxb(0.0f);
    for (auto &g : shaped)
    {
        maxb = glm::max(maxb, g.position + g.size);
    }

    return {maxb.x - shaped[0].position.x, maxb.y};
}

auto OMFontSet::genOutline(OMFontSetShapeResult r) -> std::vector<float>
{
    std::vector<glm::vec2> bufferData = {};
    std::vector<glm::vec2> beginPoints = {};
    std::vector<int> curves = {};
    std::vector<float> finalData = {};

    auto outline = r.font->buildOutline(r.glyphId, false);
    auto ext = r.font->scale();

    int vtxCount = 0;
    glm::vec2 curr = {};
    for (auto &ot : outline.operations)
    {
        switch (ot.type)
        {
        case fontproc::Move:
            beginPoints.emplace_back(ot.target / ext);
            break;
        case fontproc::Line:
            bufferData.emplace_back(ot.target / ext);
            bufferData.emplace_back(INFINITY, INFINITY);
            ++vtxCount;
            break;
        case fontproc::Cubic:
            bufferData.emplace_back(ot.target / ext);
            bufferData.emplace_back(approximateCubicToQuadratic(curr, ot.control1, ot.control2, ot.target) / ext);
            ++vtxCount;
            break;
        case fontproc::Quadratic:
            bufferData.emplace_back(ot.target / ext);
            bufferData.emplace_back(ot.control1 / ext);
            ++vtxCount;
            break;
        case fontproc::Close:
            curves.push_back(vtxCount);
            vtxCount = 0;
            continue;
        }

        curr = ot.target / ext;
    }
    auto bbox = r.font->fetchBox(r.glyphId, false);
    finalData.push_back(bbox.x);
    finalData.push_back(bbox.y);
    finalData.push_back(bbox.z);
    finalData.push_back(bbox.w);
    finalData.push_back(curves.size());
    for (auto c : curves)
    {
        finalData.push_back(c);
    }
    for (auto p : beginPoints)
    {
        finalData.push_back(p.x);
        finalData.push_back(p.y);
    }
    for (auto cc : bufferData)
    {
        finalData.push_back(cc.x);
        finalData.push_back(cc.y);
    }

    return finalData;
}
} // namespace openminecraft::fontproc
