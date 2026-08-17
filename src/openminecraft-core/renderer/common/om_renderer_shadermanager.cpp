#include "openminecraft/renderer/common/om_renderer_shadermanager.hpp"
#include "openminecraft/io/om_io_utils.hpp"
#include "openminecraft/renderer/common/basics/om_vertex_format.hpp"
#include "openminecraft/renderer/common/om_renderer_shader.hpp"
#include "openminecraft/vfs/om_vfs_base.hpp"
#include <cstdint>
#include <memory>
#include <regex>
#include <string>
#include <vector>

namespace openminecraft::renderer::common
{
static auto convertType(basics::OMVertexPropType t) -> std::string
{
    switch (t)
    {
    default:
    case basics::Float:
        return "float";
    case basics::Integer:
        return "int";
    case basics::Double:
        return "double";
    case basics::Vec2f:
        return "vec2";
    case basics::Vec3f:
        return "vec3";
    case basics::Vec4f:
        return "vec4";
    case basics::Vec2i:
        return "ivec2";
    case basics::Vec3i:
        return "ivec3";
    case basics::Vec4i:
        return "ivec4";
    case basics::Vec2d:
        return "dvec2";
    case basics::Vec3d:
        return "dvec3";
    case basics::Vec4d:
        return "dvec4";
    }
}
auto OMRendererShaderManager::preprocess(std::string name, OMShaderType type, OMShaderFileType filetype,
                                         basics::OMVertexFormat &format) -> std::shared_ptr<OMShader>
{
    logger.debug("loading {}", name);
    format.debugState();

    std::string vtxdef = "";
    int i = 0;
    for (auto &part : format.parts)
    {
        for (auto &att : part.parts)
        {
            vtxdef += fmt::format("layout(location = {}) in {} {};\n", i++,
                                  convertType(std::get<basics::OMVertexPropType>(att)), std::get<std::string>(att));
        }
    }

    auto ff = vfs::fsfetch(root + "/" + name);
    auto raw = io::readOnce(ff.get());

    std::string str(raw.begin(), raw.end());

    if (filetype == GLSLSource)
    {
        auto l = str.find("#vertex");

        if (l != std::string::npos)
        {
            str.replace(l, 7, vtxdef);
        }
    }

    std::regex include_regex("^\\s*#include\\s+(?:\"([^\"]+)\"|<([^>]+)>)", std::regex::multiline);

    while (true)
    {
        std::sregex_iterator it(str.begin(), str.end(), include_regex);
        std::sregex_iterator end;
        struct Replacement
        {
            uint64_t pos;
            uint64_t length;
            std::string filename;
        };
        std::vector<Replacement> repl = {};

        for (; it != end; ++it)
        {
            std::smatch match = *it;
            if (match[1].matched)
            {
                repl.emplace_back(Replacement{static_cast<uint64_t>(match.position()),
                                              static_cast<uint64_t>(match.length()), match[1].str()});
            }
            else if (match[2].matched)
            {
                repl.emplace_back(Replacement{static_cast<uint64_t>(match.position()),
                                              static_cast<uint64_t>(match.length()), match[2].str()});
            }
        }

        if (repl.empty())
        {
            break;
        }

        for (auto rit = repl.rbegin(); rit != repl.rend(); ++rit)
        {
            auto ff = vfs::fsfetch(root + "/" + rit->filename);
            auto raw = io::readOnce(ff.get());

            std::string strsub(raw.begin(), raw.end());
            str.replace(rit->pos, rit->length, strsub);
        }
    }

    std::vector<uint8_t> comp;
    comp.assign(str.begin(), str.end());

    return std::make_shared<OMShader>(filetype, comp, name, "main", type);
}
} // namespace openminecraft::renderer::common
