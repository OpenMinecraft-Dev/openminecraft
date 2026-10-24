#include "openminecraft/io/om_io_parser.hpp"

namespace openminecraft::io
{
OMParser::OMParser(std::shared_ptr<std::istream> stream) : source(stream.get()), sourceOwner(std::move(stream))
{
}
OMParser::~OMParser() = default;
auto OMParser::check() -> bool
{
    return source->good();
}
} // namespace openminecraft::io
