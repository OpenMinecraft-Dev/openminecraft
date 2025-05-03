#include "openminecraft/io/om_io_parser.hpp"

namespace openminecraft::io
{
OMParser::OMParser(std::shared_ptr<std::istream> stream) : source(stream)
{
}
OMParser::~OMParser()
{
}
bool OMParser::check()
{
    return source->good();
}
} // namespace openminecraft::io