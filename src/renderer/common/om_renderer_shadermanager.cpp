#include "openminecraft/renderer/common/om_renderer_shadermanager.hpp"
#include "openminecraft/io/om_io_utils.hpp"
#include "openminecraft/renderer/common/om_renderer_shader.hpp"
#include "openminecraft/vfs/om_vfs_base.hpp"
#include <cstdint>
#include <memory>
#include <regex>
#include <vector>

namespace openminecraft::renderer::common
{
auto OMRendererShaderManager::preprocess(std::string name, OMShaderType type, OMShaderFileType filetype)
    -> std::shared_ptr<OMShader>
{
    auto ff = vfs::fsfetch(root + "/" + name);
    auto raw = io::readOnce(ff.get());

    std::string str(raw.begin(), raw.end());

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
