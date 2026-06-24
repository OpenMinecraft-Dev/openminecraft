#include "openminecraft/specs/zip/om_zip.hpp"
#include <array>

namespace openminecraft::specs::zip
{
OMZip::OMZip(std::shared_ptr<std::istream> istr) : logger("OMZip", this)
{
    std::array<char, 2> header;
    istr->read(header.data(), 2);

    logger.debug("{} {}", header[0], header[1]);
}
} // namespace openminecraft::specs::zip
