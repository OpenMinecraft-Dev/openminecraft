#ifndef OM_FONTSET_HPP
#define OM_FONTSET_HPP

#include "glm/ext/vector_float2.hpp"
#include "openminecraft/geom/om_font.hpp"
#include "openminecraft/log/om_log_common.hpp"
#include <string>
#include <vector>
#include <memory>
namespace openminecraft::geom
{
struct OMFontSetShapeResult
{
    OMFont *font;
    uint32_t fontId;
    uint32_t glyphId;
    glm::vec2 position;
    glm::vec2 size;
};
class OMFontSet
{
  public:
    OMFontSet() : logger("OMFontSet", this)
    {
    }
    ~OMFontSet() = default;

    auto shape(std::string s) -> std::vector<OMFontSetShapeResult>;

    // Pen advance width of the string (relative to the first glyph's ink),
    // including trailing spaces and advances that carry no visible ink.
    auto advance(std::string s) -> float;
    auto compile(OMFontSetShapeResult) -> std::vector<float>;
    auto bound(std::vector<OMFontSetShapeResult>) -> glm::vec2;

    std::vector<std::shared_ptr<OMFont>> fontList;

  private:
    log::OMLogger logger;
};
} // namespace openminecraft::geom

#endif
