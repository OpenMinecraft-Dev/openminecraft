#ifndef OM_UTIL_STRING_HPP
#define OM_UTIL_STRING_HPP

#include <istream>
#include <memory>
#include <vector>
namespace openminecraft::util::string
{
auto utf8Next(std::shared_ptr<std::istream> s) -> int;
auto uniToString(std::vector<int> arr) -> std::string;
} // namespace openminecraft::util::string

#endif
