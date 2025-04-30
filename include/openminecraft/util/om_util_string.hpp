#ifndef OM_UTIL_STRING_HPP
#define OM_UTIL_STRING_HPP

#include <istream>
#include <memory>
#include <vector>
namespace openminecraft::util::string
{
int utf8Next(std::shared_ptr<std::istream> s);
std::string uniToString(std::vector<int> arr);
} // namespace openminecraft::util::string

#endif