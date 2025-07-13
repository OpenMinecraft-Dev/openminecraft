#ifndef OM_PIXELTOWER_CLASS_STRUCTDEF_HPP
#define OM_PIXELTOWER_CLASS_STRUCTDEF_HPP

#include <any>
#include <cstdint>
#include <list>
#include <memory>
#include <string>
#include <vector>
namespace openminecraft::vm::pixeltower
{
enum OMInstanceType
{
    Byte,
    Short,
    Int,
    Long,
    Float,
    Double,
    Boolean,
    Char,
    Reference,
    Array
};
struct OMFieldInfo
{
    uint16_t accessFlag;
    std::string name;
    OMInstanceType type;
    std::any ref;
    std::string rawDesc;
    uint64_t offset;
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
    uint64_t objectLength = 0;

    void *staticFieldBlock = nullptr;
};
} // namespace openminecraft::vm::pixeltower

#endif