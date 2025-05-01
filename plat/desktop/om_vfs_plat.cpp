#include "fmt/format.h"
#include "openminecraft/vfs/om_vfs_base.hpp"
#include <filesystem>

namespace openminecraft::vfs
{
bool fsmountAssets(std::string mountpoint)
{
    return fsmountReal(fmt::format("{}/assets", std::filesystem::current_path().string()), mountpoint);
}
} // namespace openminecraft::vfs