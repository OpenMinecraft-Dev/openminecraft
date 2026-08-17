#include "openminecraft/io/om_io_parser.hpp"

namespace openminecraft::io
{
OMParser::OMParser(std::istream *stream) : source(stream)
{
}
OMParser::~OMParser() = default;
auto OMParser::check() -> bool
{
    return source->good();
}
} // namespace openminecraft::io
