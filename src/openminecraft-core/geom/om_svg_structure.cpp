#include "openminecraft/geom/om_svg_structure.hpp"
#include "glm/fwd.hpp"
#include <cmath>
#include <utility>
#include <vector>
#include <string>
#include <cctype>

namespace openminecraft::geom::svg
{
// INFO: svg outline compiled structure
// Chunk 1: glyph count (1 float)
// Chunk 2: glyph data
// begin point (2 floats)
// curve count (1 float)
// for every curve:
// curve type (1 float)
// type == 0: (Straightline)
// target (2 floats)
// type == 1: (Quadratic)
// target (2 floats)
// controlPoint1 (2 floats)
// type == 2: (Cubic)
// target (2 floats)
// controlPoint1 (2 floats)
// controlPoint2 (2 floats)
// type == 3:
// target (2 floats)
// ellipse args (rx, ry, xRot, 3 floats)
// largeArc + sweep (1 float)
auto compile(std::vector<std::pair<glm::vec2, std::vector<OMSvgCurve>>> r, glm::vec2 siz) -> std::vector<float>
{
    std::vector<float> data = {};
    data.push_back(r.size());

    for (const auto &p : r)
    {
        data.push_back(p.first.x / siz.x);
        data.push_back(p.first.y / siz.y);
        data.push_back(p.second.size());

        for (const auto &cur : p.second)
        {
            data.push_back(cur.op);
            switch (cur.op)
            {
            case StraightLine:
                data.push_back(cur.end.x / siz.x);
                data.push_back(cur.end.y / siz.y);
                break;
            case Quadratic:
                data.push_back(cur.end.x / siz.x);
                data.push_back(cur.end.y / siz.y);
                data.push_back(cur.control1.x / siz.x);
                data.push_back(cur.control1.y / siz.y);
                break;
            case Cubic:
                data.push_back(cur.end.x / siz.x);
                data.push_back(cur.end.y / siz.y);
                data.push_back(cur.control1.x / siz.x);
                data.push_back(cur.control1.y / siz.y);
                data.push_back(cur.control2.x / siz.x);
                data.push_back(cur.control2.y / siz.y);
                break;
            case Arc:
                data.push_back(cur.end.x / siz.x);
                data.push_back(cur.end.y / siz.y);
                data.push_back(cur.rx / siz.x);
                data.push_back(cur.ry / siz.y);
                data.push_back(cur.xRot);
                data.push_back(cur.largeArcFlag << 1 | cur.sweepFlag);
                break;
            }
        }
    }

    return data;
}
auto mergeTo(std::vector<OMSvgPathSegment> p) -> std::vector<std::pair<glm::vec2, std::vector<OMSvgCurve>>>
{
    glm::vec2 current = {NAN, NAN}, lastCtrl = {NAN, NAN};
    std::vector<std::pair<glm::vec2, std::vector<OMSvgCurve>>> result = {};
    for (const auto &seg : p)
    {
        switch (seg.op)
        {
        case MoveTo:
            current = seg.move.target;
            result.push_back({current, {}});
            lastCtrl = current;
            break;
        case RelativeMoveTo:
            current += seg.move.target;
            result.push_back({current, {}});
            lastCtrl = current;
            break;
        case LineTo:
            current = seg.line.target;
            result.back().second.push_back({StraightLine, current});
            lastCtrl = current;
            break;
        case RelativeLineTo:
            current += seg.line.target;
            result.back().second.push_back({StraightLine, current});
            lastCtrl = current;
            break;
        case HorizontalLineTo:
            current.x = seg.hline.x;
            result.back().second.push_back({StraightLine, current});
            lastCtrl = current;
            break;
        case RelativeHorizontalLineTo:
            current.x += seg.hline.x;
            result.back().second.push_back({StraightLine, current});
            lastCtrl = current;
            break;
        case VerticalLineTo:
            current.y = seg.vline.y;
            result.back().second.push_back({StraightLine, current});
            lastCtrl = current;
            break;
        case RelativeVerticalLineTo:
            current.y += seg.vline.y;
            result.back().second.push_back({StraightLine, current});
            lastCtrl = current;
            break;
        case CubicTo:
            current = seg.cubic.target;
            result.back().second.push_back({Cubic, current, seg.cubic.control1, seg.cubic.control2});
            lastCtrl = seg.cubic.control2;
            break;
        case RelativeCubicTo:
            result.back().second.push_back(
                {Cubic, current + seg.cubic.target, current + seg.cubic.control1, current + seg.cubic.control2});
            lastCtrl = current + seg.cubic.control2;
            current += seg.cubic.target;
            break;
        case SmoothCubicTo:
            result.back().second.push_back(
                {Cubic, seg.smoothCubic.target, current * glm::vec2(2) - lastCtrl, seg.smoothCubic.control1});
            lastCtrl = seg.smoothCubic.control1;
            current = seg.smoothCubic.target;
            break;
        case RelativeSmoothCubicTo:
            result.back().second.push_back({Cubic, current + seg.smoothCubic.target, current * glm::vec2(2) - lastCtrl,
                                            current + seg.smoothCubic.control1});
            lastCtrl = current + seg.smoothCubic.control1;
            current += seg.smoothCubic.target;
            break;
        case QuadraticTo:
            current = seg.quadratic.target;
            result.back().second.push_back({Quadratic, current, seg.quadratic.control1});
            lastCtrl = seg.quadratic.control1;
            break;
        case RelativeQuadraticTo:
            result.back().second.push_back(
                {Quadratic, current + seg.quadratic.target, current + seg.quadratic.control1});
            lastCtrl = current + seg.quadratic.control1;
            current += seg.quadratic.target;
            break;
        case SmoothQuadraticTo:
            result.back().second.push_back({Quadratic, seg.smoothQuadratic.target, current * glm::vec2(2) - lastCtrl});
            lastCtrl = current * glm::vec2(2) - lastCtrl;
            current = seg.smoothQuadratic.target;
            break;
        case RelativeSmoothQuadraticTo:
            result.back().second.push_back(
                {Quadratic, current + seg.smoothQuadratic.target, current * glm::vec2(2) - lastCtrl});
            lastCtrl = current * glm::vec2(2) - lastCtrl;
            current += seg.smoothQuadratic.target;
            break;
        case ArcTo:
            current = seg.arc.target;
            result.back().second.push_back({Arc, current, glm::vec2(), glm::vec2(), seg.arc.rx, seg.arc.ry,
                                            seg.arc.xRot, seg.arc.largeArcFlag, seg.arc.sweepFlag});
            lastCtrl = current;
            break;
        case RelativeArcTo:
            current += seg.arc.target;
            result.back().second.push_back({Arc, current, glm::vec2(), glm::vec2(), seg.arc.rx, seg.arc.ry,
                                            seg.arc.xRot, seg.arc.largeArcFlag, seg.arc.sweepFlag});
            lastCtrl = current;
            break;
        case Close:
            current = result.back().first;
            result.back().second.push_back({StraightLine, current});
            lastCtrl = current;
            break;
        }
    }
    return result;
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
        if (it->at(0) == 'z' || it->at(0) == 'Z')
        {
            target.push_back({Close});
        }
        else
        {
            char op = it->at(0);
            ++it;
            auto a = parseFloatArr(*it);
            int paramsPerCmd = 0;
            switch (op)
            {
            case 'M':
            case 'm':
            case 'L':
            case 'l':
            case 'T':
            case 't':
                paramsPerCmd = 2;
                break;
            case 'H':
            case 'h':
            case 'V':
            case 'v':
                paramsPerCmd = 1;
                break;
            case 'C':
            case 'c':
                paramsPerCmd = 6;
                break;
            case 'S':
            case 's':
            case 'Q':
            case 'q':
                paramsPerCmd = 4;
                break;
            case 'A':
            case 'a':
                paramsPerCmd = 7;
                break;
            default:
                paramsPerCmd = 0;
                break;
            }

            if (paramsPerCmd <= 0)
                continue; // 无效命令，跳过

            int numGroups = a.size() / paramsPerCmd;
            if (numGroups == 0)
                continue; // 没有参数

            for (int gi = 0; gi < numGroups; ++gi)
            {
                OMSvgPathSegment seg;
                int offset = gi * paramsPerCmd;
                switch (op)
                {
                case 'M':
                    if (gi == 0)
                    {
                        seg.op = MoveTo;
                        seg.move = {{a[offset], a[offset + 1]}};
                    }
                    else
                    {
                        seg.op = LineTo; // 后续组隐式转换为 LineTo
                        seg.line = {{a[offset], a[offset + 1]}};
                    }
                    break;
                case 'm':
                    if (gi == 0)
                    {
                        seg.op = RelativeMoveTo;
                        seg.move = {{a[offset], a[offset + 1]}};
                    }
                    else
                    {
                        seg.op = RelativeLineTo;
                        seg.line = {{a[offset], a[offset + 1]}};
                    }
                    break;
                case 'L':
                    seg.op = LineTo;
                    seg.line = {{a[offset], a[offset + 1]}};
                    break;
                case 'l':
                    seg.op = RelativeLineTo;
                    seg.line = {{a[offset], a[offset + 1]}};
                    break;
                case 'H':
                    seg.op = HorizontalLineTo;
                    seg.hline = {a[offset]};
                    break;
                case 'h':
                    seg.op = RelativeHorizontalLineTo;
                    seg.hline = {a[offset]};
                    break;
                case 'V':
                    seg.op = VerticalLineTo;
                    seg.vline = {a[offset]};
                    break;
                case 'v':
                    seg.op = RelativeVerticalLineTo;
                    seg.vline = {a[offset]};
                    break;
                case 'C':
                    seg.op = CubicTo;
                    seg.cubic = {
                        {a[offset], a[offset + 1]}, {a[offset + 2], a[offset + 3]}, {a[offset + 4], a[offset + 5]}};
                    break;
                case 'c':
                    seg.op = RelativeCubicTo;
                    seg.cubic = {
                        {a[offset], a[offset + 1]}, {a[offset + 2], a[offset + 3]}, {a[offset + 4], a[offset + 5]}};
                    break;
                case 'S':
                    seg.op = SmoothCubicTo;
                    seg.smoothCubic = {{a[offset], a[offset + 1]}, {a[offset + 2], a[offset + 3]}};
                    break;
                case 's':
                    seg.op = RelativeSmoothCubicTo;
                    seg.smoothCubic = {{a[offset], a[offset + 1]}, {a[offset + 2], a[offset + 3]}};
                    break;
                case 'Q':
                    seg.op = QuadraticTo;
                    seg.quadratic = {{a[offset], a[offset + 1]}, {a[offset + 2], a[offset + 3]}};
                    break;
                case 'q':
                    seg.op = RelativeQuadraticTo;
                    seg.quadratic = {{a[offset], a[offset + 1]}, {a[offset + 2], a[offset + 3]}};
                    break;
                case 'T':
                    seg.op = SmoothQuadraticTo;
                    seg.smoothQuadratic = {{a[offset], a[offset + 1]}};
                    break;
                case 't':
                    seg.op = RelativeSmoothQuadraticTo;
                    seg.smoothQuadratic = {{a[offset], a[offset + 1]}};
                    break;
                case 'A':
                    seg.op = ArcTo;
                    seg.arc = {a[offset],
                               a[offset + 1],
                               a[offset + 2],
                               static_cast<bool>(a[offset + 3]),
                               static_cast<bool>(a[offset + 4]),
                               {a[offset + 5], a[offset + 6]}};
                    break;
                case 'a':
                    seg.op = RelativeArcTo;
                    seg.arc = {a[offset],
                               a[offset + 1],
                               a[offset + 2],
                               static_cast<bool>(a[offset + 3]),
                               static_cast<bool>(a[offset + 4]),
                               {a[offset + 5], a[offset + 6]}};
                    break;
                default:
                    break;
                }
                target.push_back(seg);
            }
        }
    }
    return target;
}
} // namespace openminecraft::geom::svg