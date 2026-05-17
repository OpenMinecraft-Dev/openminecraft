#ifndef OM_ENCODING_UTF_HPP
#define OM_ENCODING_UTF_HPP
#include "openminecraft/vm/elysia/interface/om_elysia_interface_defs.hpp"
#include "openminecraft/vm/elysia/om_elysia_types.hpp"
#include <cstdint>
#include <string>
#include <vector>

namespace openminecraft::vm::encoding
{
std::vector<int> utf16ToUtf32(std::vector<uint8_t>);
std::string utf32ToUtf8(std::vector<int> cps);

std::tuple<elysia::jchar *, elysia::jsize> utf8ToUtf16New(std::string);
std::string utf16ToUtf8New(elysia::jchar *, elysia::jsize);
} // namespace openminecraft::vm::encoding

#endif
