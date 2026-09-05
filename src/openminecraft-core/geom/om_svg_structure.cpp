#include "openminecraft/geom/om_svg_structure.hpp"
#include <cmath>
#include <vector>
#include <string>
#include <cctype>

namespace openminecraft::geom::svg
{
auto mergeTo(std::vector<OMSvgPathSegment> p) -> std::vector<OMSvgCurve>
{
    glm::vec2 current = {NAN, NAN};
    std::vector<OMSvgCurve> result = {};
    for (const auto &seg : p)
    {
    }
    return {};
}
auto parseFloatArr(std::string p) -> std::vector<float>
{
    std::vector<float> result;
    std::string current;
    bool hasDot = false;

    auto flush = [&]() {
        if (!current.empty())
        {
            try
            {
                result.push_back(std::stof(current));
            }
            catch (...)
            {
            }
            current.clear();
            hasDot = false;
        }
    };

    for (char c : p)
    {
        if (c == '+' || c == '-')
        {
            flush();
            current += c;
            hasDot = false;
        }
        else if (c == '.')
        {
            if (hasDot)
            {
                flush();
                current += c;
                hasDot = true;
            }
            else
            {
                current += c;
                hasDot = true;
            }
        }
        else if (std::isdigit(static_cast<unsigned char>(c)))
        {
            current += c;
        }
        else
        {
            flush();
        }
    }
    flush();

    return result;
}
auto parseSvgPath(std::string p) -> std::vector<OMSvgPathSegment>
{
    std::vector<std::string> segs = {};
    std::string args = "";
    for (auto c : p)
    {
        if ((c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z'))
        {
            if (args.size() > 0)
            {
                segs.push_back(args);
            }
            args = "";
            segs.emplace_back(&c);
        }
        else
        {
            args += c;
        }
    }

    std::vector<OMSvgPathSegment> target = {};
    for (auto it = segs.begin(); it != segs.end(); ++it)
    {
        if (*it == "z" || *it == "Z")
        {
            target.push_back({Close});
        }
        else
        {
            auto op = it->at(0);
            ++it;
            auto a = parseFloatArr(*it);
            OMSvgPathSegment seg;
            switch (op)
            {
            case 'M':
                seg.op = MoveTo;
                seg.move = {{a[0], a[1]}};
                break;
            case 'm':
                seg.op = RelativeMoveTo;
                seg.move = {{a[0], a[1]}};
                break;
            case 'L':
                seg.op = LineTo;
                seg.move = {{a[0], a[1]}};
                break;
            case 'l':
                seg.op = RelativeLineTo;
                seg.move = {{a[0], a[1]}};
                break;
            case 'H':
                seg.op = HorizontalLineTo;
                seg.hline = {a[0]};
                break;
            case 'h':
                seg.op = RelativeHorizontalLineTo;
                seg.hline = {a[0]};
                break;
            case 'V':
                seg.op = VerticalLineTo;
                seg.vline = {a[0]};
                break;
            case 'v':
                seg.op = RelativeVerticalLineTo;
                seg.vline = {a[0]};
                break;
            case 'C':
                seg.op = CubicTo;
                seg.cubic = {{a[0], a[1]}, {a[2], a[3]}, {a[4], a[5]}};
                break;
            case 'c':
                seg.op = RelativeCubicTo;
                seg.cubic = {{a[0], a[1]}, {a[2], a[3]}, {a[4], a[5]}};
                break;
            case 'S':
                seg.op = SmoothCubicTo;
                seg.smoothCubic = {{a[0], a[1]}, {a[2], a[3]}};
                break;
            case 's':
                seg.op = RelativeSmoothCubicTo;
                seg.smoothCubic = {{a[0], a[1]}, {a[2], a[3]}};
                break;
            case 'Q':
                seg.op = QuadraticTo;
                seg.quadratic = {{a[0], a[1]}, {a[2], a[3]}};
                break;
            case 'q':
                seg.op = RelativeQuadraticTo;
                seg.quadratic = {{a[0], a[1]}, {a[2], a[3]}};
                break;
            case 'T':
                seg.op = SmoothQuadraticTo;
                seg.smoothQuadratic = {{a[0], a[1]}};
                break;
            case 't':
                seg.op = RelativeSmoothQuadraticTo;
                seg.smoothQuadratic = {{a[0], a[1]}};
                break;
            case 'A':
                seg.op = ArcTo;
                seg.arc = {a[0], a[1], a[2], static_cast<bool>(a[3]), static_cast<bool>(a[4]), {a[5], a[6]}};
                break;
            case 'a':
                seg.op = RelativeArcTo;
                seg.arc = {a[0], a[1], a[2], static_cast<bool>(a[3]), static_cast<bool>(a[4]), {a[5], a[6]}};
                break;
            default:
                break;
            }
            target.push_back(seg);
        }
    }
    return target;
}
} // namespace openminecraft::geom::svg