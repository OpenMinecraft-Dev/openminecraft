#ifndef OM_PIXELTOWER_KLASS_HPP
#define OM_PIXELTOWER_KLASS_HPP

#include "openminecraft/vm/classfile/om_class_file.hpp"
#include "openminecraft/vm/pixeltower/v0/om_pixeltower_base.hpp"
#include <cstdint>
#include <memory>
#include <string>
namespace openminecraft::vm::pixeltower::v0
{
enum OMKlassKind : uint8_t
{
    Normal,
    Array
};

class OMKlass
{
  public:
    OMKlassKind kind;
    std::string name;
    std::shared_ptr<classfile::OMClassFile> raw;
    OMKlass *superClass;
    AccessFlags accessFlags;
};
} // namespace openminecraft::vm::pixeltower::v0

#endif