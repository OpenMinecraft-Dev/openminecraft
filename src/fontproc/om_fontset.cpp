#include "openminecraft/fontproc/om_fontset.hpp"
#include "openminecraft/fontproc/om_font.hpp"
#include "openminecraft/fontproc/om_font_outline.hpp"
#include <vector>
#include <glm/glm.hpp>

namespace openminecraft::fontproc
{
auto OMFontSet::shape(std::string s) -> std::vector<OMFontShapeResult>
{
    std::vector<OMFontShapeResult> glyphs = {};
    for (auto f : fontList)
    {
        logger.info("shaping with font {}", (void *)f.get());
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
            }
        }
    }

    for (auto &c : glyphs)
    {
        logger.info("{} Cluster {} glyphid 0x{:x} + offset({}, {}) / advance({}, {})", (void *)c.font, c.cluster,
                    c.glyphId, c.offsetx, c.offsety, c.advancex, c.advancey);
    }
    return glyphs;
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

auto OMFontSet::genOutline(OMFont *fnt, int glyphId) -> std::vector<float>
{
    std::vector<glm::vec2> bufferData = {};
    std::vector<glm::vec2> beginPoints = {};
    std::vector<int> curves = {};
    std::vector<float> finalData = {};

    auto outline = fnt->buildOutline(glyphId, false);
    auto ext = fnt->scale();

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
    auto bbox = fnt->fetchBox(glyphId);

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
