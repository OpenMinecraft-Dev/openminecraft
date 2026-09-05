#ifndef OM_FONT_HPP
#define OM_FONT_HPP
#include "openminecraft/log/om_log_common.hpp"
#include <cstdint>
#include <iosfwd>
#include <glm/glm.hpp>
#include <vector>

namespace openminecraft::fontproc
{
constexpr const char allocatorId[] = "font_processor";
class OMFontOutline;
class OMFont;
struct OMFontShapeResult
{
    uint32_t glyphId, cluster;
    float offsetx;
    float offsety;
    float advancex;
    float advancey;
    OMFont *font;
    uint32_t fontId;
};
class OMFont
{
  public:
    OMFont(std::istream &istr);
    ~OMFont();

    auto scale() -> glm::vec2;
    auto buildOutline(int charcode, bool uni = true) -> OMFontOutline;
    auto fetchBox(int charcode, bool uni = true) -> glm::vec4;
    auto shape(std::string s, bool &isRTL) -> std::vector<OMFontShapeResult>;
    auto metrics(bool) -> glm::vec4;

  private:
    void *hbFont;
    void *hbFace;

    log::OMLogger logger;
};
} // namespace openminecraft::fontproc

#endif
