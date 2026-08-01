#include "openminecraft/renderer/common/om_renderer_shadermanager.hpp"
#include "openminecraft/io/om_io_utils.hpp"
#include "openminecraft/renderer/common/om_renderer_shader.hpp"
#include "openminecraft/vfs/om_vfs_base.hpp"
#include <cstdint>
#include <memory>

namespace openminecraft::renderer::common
{
auto OMRendererShaderManager::preprocess(std::string name, OMShaderType type, OMShaderFileType filetype)
    -> std::shared_ptr<OMShader>
{
    auto ff = vfs::fsfetch(root + "/" + name);
    auto raw = io::readOnce(ff.get());

    std::string str(raw.begin(), raw.end());

    std::vector<uint8_t> comp;
    comp.assign(str.begin(), str.end());

    return std::make_shared<OMShader>(filetype, comp, name, "main", type);
}
} // namespace openminecraft::renderer::common
