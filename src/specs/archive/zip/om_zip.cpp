#include "openminecraft/specs/zip/om_zip.hpp"

namespace openminecraft::specs::zip
{
OMZip::OMZip(std::shared_ptr<std::istream> istr) : logger("OMZip", this)
{
}
} // namespace openminecraft::specs::zip
