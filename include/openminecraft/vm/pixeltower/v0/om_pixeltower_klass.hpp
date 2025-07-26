#ifndef OM_PIXELTOWER_KLASS_HPP
#define OM_PIXELTOWER_KLASS_HPP

#include "openminecraft/vm/classfile/om_class_file.hpp"
#include "openminecraft/vm/pixeltower/v0/om_pixeltower_base.hpp"
#include "openminecraft/vm/pixeltower/v0/om_pixeltower_field.hpp"
#include "openminecraft/vm/pixeltower/v0/om_pixeltower_heap.hpp"
#include "openminecraft/vm/pixeltower/v0/om_pixeltower_oop.hpp"
#include <cassert>
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

struct OMMethod;

class OMKlass
{
  public:
    OMKlassKind kind;
    std::string name;
    std::shared_ptr<classfile::OMClassFile> raw;
    OMKlass *superClass;
    std::vector<OMKlass *> interfaces;
    OMMethod *methods;
    AccessFlags accessFlags;
    std::vector<OMField> fields;
    uint64_t length;
    void *staticBlock;
    uint64_t staticLength;

    OMPixelTowerHeap *heap;

    OMOOPDesc *allocateInstance()
    {
        assert(kind == OMKlassKind::Normal);
        auto obj = (OMOOPDesc *)heap->allocate(sizeof(OMOOPDesc) + length);
        obj->klass = this;
        return obj;
    }
};
} // namespace openminecraft::vm::pixeltower::v0

#endif