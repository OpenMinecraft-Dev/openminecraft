#ifndef OM_SVG_STRUCTURE
#define OM_SVG_STRUCTURE

#include "glm/ext/vector_float2.hpp"
#include <string>
#include <vector>
namespace openminecraft::geom::svg
{
enum OMSvgCurveType
{
    StraightLine = 0,
    Quadratic = 1,
    Cubic = 2,
    Arc = 3
};
struct OMSvgCurve
{
    OMSvgCurveType op;
    glm::vec2 end;

    glm::vec2 control1;
    glm::vec2 control2;
    float rx;
    float ry;
    float xRot;
    bool largeArcFlag;
    bool sweepFlag;
};
enum OMSvgOperations
{
    MoveTo,
    RelativeMoveTo,
    LineTo,
    RelativeLineTo,
    HorizontalLineTo,
    RelativeHorizontalLineTo,
    VerticalLineTo,
    RelativeVerticalLineTo,
    CubicTo,
    RelativeCubicTo,
    SmoothCubicTo,
    RelativeSmoothCubicTo,
    QuadraticTo,
    RelativeQuadraticTo,
    SmoothQuadraticTo,
    RelativeSmoothQuadraticTo,
    ArcTo,
    RelativeArcTo,
    Close
};

struct OMSvgPathSegment
{
    OMSvgOperations op;
    union {
        struct
        {
            glm::vec2 target;
        } move;

        struct
        {
            glm::vec2 target;
        } line;

        struct
        {
            float x;
        } hline;

        struct
        {
            float y;
        } vline;

        struct
        {
            glm::vec2 control1;
            glm::vec2 control2;
            glm::vec2 target;
        } cubic;

        struct
        {
            glm::vec2 control1;
            glm::vec2 target;
        } smoothCubic;

        struct
        {
            glm::vec2 control1;
            glm::vec2 target;
        } quadratic;

        struct
        {
            glm::vec2 target;
        } smoothQuadratic;

        struct
        {
            float rx;
            float ry;
            float xRot;
            bool largeArcFlag;
            bool sweepFlag;
            glm::vec2 target;
        } arc;
    };
};

auto parseSvgPath(std::string) -> std::vector<OMSvgPathSegment>;
auto parseFloatArr(std::string) -> std::vector<float>;
auto mergeTo(std::vector<OMSvgPathSegment>) -> std::vector<std::pair<glm::vec2, std::vector<OMSvgCurve>>>;
auto compile(std::vector<std::pair<glm::vec2, std::vector<OMSvgCurve>>> r, glm::vec2 siz) -> std::vector<float>;
} // namespace openminecraft::geom::svg

#endif
