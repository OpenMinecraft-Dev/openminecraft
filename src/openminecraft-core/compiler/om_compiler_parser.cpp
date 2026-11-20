#include "openminecraft/compiler/om_compiler_parser.hpp"

namespace openminecraft::compiler
{
OMParser::OMParser(std::shared_ptr<std::istream> stream) : source(stream.get()), sourceOwner(std::move(stream))
{
}
OMParser::~OMParser() = default;
auto OMParser::check() -> bool
{
    return source->good();
}
} // namespace openminecraft::compiler
