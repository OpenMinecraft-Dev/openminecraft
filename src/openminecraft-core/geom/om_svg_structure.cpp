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
            default:
                break;
            }
        }
    }
    return target;
}
} // namespace openminecraft::geom