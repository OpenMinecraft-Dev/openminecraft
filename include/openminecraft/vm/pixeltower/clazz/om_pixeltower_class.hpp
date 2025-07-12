#ifndef OM_PIXELTOWER_CLASS_HPP
#define OM_PIXELTOWER_CLASS_HPP

#include "openminecraft/vm/classfile/om_class_file.hpp"
#include <bitset>
#include <string>
#include <unordered_map>
namespace openminecraft::vm::pixeltower
{
constexpr int publicAccessBit = 0;
constexpr int privateAccessBit = 1;
constexpr int protectedAccessBit = 2;
constexpr int defaultAccessBit = 3;

struct OMMethodInfo
{
    std::string desc;
    std::bitset<16> flags;
    std::unordered_map<uint64_t, uint64_t> debugMapping;
    vm::classfile::OMClassAttrCode *code;
};

class OMClass
{
  public:
    OMClass();
    ~OMClass();

    std::string source;
    std::unordered_map<std::string, OMMethodInfo> methods;
    std::unordered_map<uint16_t, std::shared_ptr<classfile::OMClassConstant>> *mapping;
};
} // namespace openminecraft::vm::pixeltower

#endif