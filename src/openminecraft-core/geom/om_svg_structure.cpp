#include "openminecraft/geom/om_svg_structure.hpp"
#include <iostream>
#include <unordered_map>
#include <utility>
#include <vector>
#include <string>
#include <cctype>

namespace openminecraft::geom
{
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
            target.push_back({.op = Close});
        }
        else
        {
            auto op = it->at(0);
            ++it;
            auto a = parseFloatArr(*it);
            switch (op)
            {
            case 'M':
                target.push_back({.op = MoveTo, .move = {{a[0], a[1]}}});
                break;
            case 'm':
                target.push_back({.op = RelativeMoveTo, .move = {{a[0], a[1]}}});
                break;
            case 'L':
                target.push_back({.op = LineTo, .line = {{a[0], a[1]}}});
                break;
            case 'l':
                target.push_back({.op = RelativeLineTo, .line = {{a[0], a[1]}}});
                break;
            case 'H':
                target.push_back({.op = HorizontalLineTo, .hline = {a[0]}});
                break;
            case 'h':
                target.push_back({.op = RelativeHorizontalLineTo, .hline = {a[0]}});
                break;
            case 'V':
                target.push_back({.op = VerticalLineTo, .hline = {a[0]}});
                break;
            case 'v':
                target.push_back({.op = RelativeVerticalLineTo, .hline = {a[0]}});
                break;
            case 'C':
                target.push_back({.op = CubicTo, .cubic = {{a[0], a[1]}, {a[2], a[3]}, {a[4], a[5]}}});
                break;
            case 'c':
                target.push_back({.op = RelativeCubicTo, .cubic = {{a[0], a[1]}, {a[2], a[3]}, {a[4], a[5]}}});
                break;
            case 'S':
                target.push_back({.op = SmoothCubicTo, .smoothCubic = {{a[0], a[1]}, {a[2], a[3]}}});
                break;
            case 's':
                target.push_back({.op = RelativeSmoothCubicTo, .smoothCubic = {{a[0], a[1]}, {a[2], a[3]}}});
                break;
            case 'Q':
                target.push_back({.op = QuadraticTo, .quadratic = {{a[0], a[1]}, {a[2], a[3]}}});
                break;
            case 'q':
                target.push_back({.op = RelativeQuadraticTo, .quadratic = {{a[0], a[1]}, {a[2], a[3]}}});
                break;
            case 'T':
                target.push_back({.op = SmoothQuadraticTo, .smoothQuadratic = {{a[0], a[1]}}});
                break;
            case 't':
                target.push_back({.op = RelativeSmoothQuadraticTo, .smoothQuadratic = {{a[0], a[1]}}});
                break;
            case 'A':
                target.push_back(
                    {.op = ArcTo,
                     .arc = {a[0], a[1], a[2], static_cast<bool>(a[3]), static_cast<bool>(a[4]), {a[5], a[6]}}});
                break;
            case 'a':
                target.push_back(
                    {.op = RelativeArcTo,
                     .arc = {a[0], a[1], a[2], static_cast<bool>(a[3]), static_cast<bool>(a[4]), {a[5], a[6]}}});
                break;
            default:
                break;
            }
        }
    }
    return target;
}
} // namespace openminecraft::geom