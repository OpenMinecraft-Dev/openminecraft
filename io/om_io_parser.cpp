#include "openminecraft/io/om_io_parser.hpp"

namespace openminecraft::io
{
OMParser::OMParser(std::istream &stream) : source(&stream)
{
}
OMParser::~OMParser()
{
}
} // namespace openminecraft::io