#ifndef OM_ENCODING_UTF_HPP
#define OM_ENCODING_UTF_HPP
#include "openminecraft/vm/elysia/interface/om_elysia_interface_defs.hpp"
#include <string>
#include <vector>

namespace openminecraft::vm::encoding
{
std::tuple<elysia::jchar *, elysia::jsize> utf8ToUtf16New(std::string str);
std::string utf16ToUtf8New(elysia::jchar *arr, elysia::jsize length);
std::string utf32ToUtf8(std::vector<int> cps);
} // namespace openminecraft::vm::encoding

#endif
