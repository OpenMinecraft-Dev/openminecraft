#ifndef OM_ENCODING_UTF_HPP
#define OM_ENCODING_UTF_HPP
#include <cstdint>
#include <string>
#include <vector>

namespace openminecraft::vm::encoding
{
std::vector<int> utf8ToUtf32(std::string n);
std::vector<uint8_t> utf32ToUtf16(std::vector<int>);
std::vector<int> utf16ToUtf32(std::vector<uint8_t>);
std::string utf32ToUtf8(std::vector<int> cps);
} // namespace openminecraft::vm::encoding

#endif