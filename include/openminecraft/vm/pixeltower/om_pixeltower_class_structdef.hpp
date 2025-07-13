#ifndef OM_PIXELTOWER_CLASS_STRUCTDEF_HPP
#define OM_PIXELTOWER_CLASS_STRUCTDEF_HPP

#include "openminecraft/vm/classfile/om_class_file.hpp"
#include <any>
#include <cstdint>
#include <list>
#include <memory>
#include <string>
#include <vector>
namespace openminecraft::vm::pixeltower
{
enum OMFieldType
{
    Bytes4,
    Bytes8,
    BytesP
};
struct OMFieldInfo
{
    uint16_t accessFlag;
    std::string name;
    std::string desc;
    OMFieldType type;
    uint64_t offset;
};
struct OMMethodInfo
{
    uint16_t accessFlag;
    std::string name;
    std::string desc;
    classfile::OMClassAttrCode *code;
};
class OMClass
{
  public:
    OMClass();
    ~OMClass();

    void calcFieldOffsets();

    std::string name;
    std::string sourceFile;
    std::shared_ptr<OMClass> superClass;
    std::vector<std::shared_ptr<OMClass>> interfaces;
    std::list<OMFieldInfo> fields;
    std::list<OMMethodInfo> methods;
    uint64_t objectLength = 0;

    void *staticFieldBlock = nullptr;
};
} // namespace openminecraft::vm::pixeltower

#endif