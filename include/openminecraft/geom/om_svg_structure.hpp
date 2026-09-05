#ifndef OM_SVG_STRUCTURE
#define OM_SVG_STRUCTURE

namespace openminecraft::geom
{
enum OMSvgOperations
{
    MoveTo,
    LineTo,
    HorizontalLineTo,
    VerticalLineTo,
    CubicTo,
    SmoothCubicTo,
    QuadraticTo,
    SmoothQuadraticTo,
    ArcTo,
    Close
};
} // namespace openminecraft::geom

#endif
