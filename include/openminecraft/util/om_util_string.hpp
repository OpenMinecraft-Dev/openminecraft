#ifndef OM_UTIL_STRING_HPP
#define OM_UTIL_STRING_HPP

#include <istream>
#include <memory>
namespace openminecraft::util::string
{
int utf8Next(std::shared_ptr<std::istream> s);
}

#endif