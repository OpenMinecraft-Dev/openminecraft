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
    Interface,
    AbstractClass,
    Annotation,
    Enum,
    Array
};

struct OMMethod;

class OMKlass
{
  public:
    OMKlassKind kind;
    std::string name;
    std::shared_ptr<classfile::OMClassFile> raw;
    OMOOPDesc *oop;
    OMKlass *superClass;
    std::vector<OMKlass *> interfaces;
    OMMethod *methods;
    std::unordered_map<std::string, OMMethod *> *vtable;
    AccessFlags accessFlags;
    std::vector<OMField> fields;
    uint64_t length;
    void *staticBlock;
    uint64_t staticLength;

    void **constantPool;

    OMPixelTowerHeap *heap;

    std::string source;

    [[nodiscard]] bool isIntArr() const
    {
        return name == "[I";
    }
    [[nodiscard]] bool isFloatArr() const
    {
        return name == "[F";
    }
    [[nodiscard]] bool isLongArr() const
    {
        return name == "[J";
    }
    [[nodiscard]] bool isDoubleArr() const
    {
        return name == "[D";
    }
    [[nodiscard]] bool isCharArr() const
    {
        return name == "[C";
    }
    [[nodiscard]] bool isShortArr() const
    {
        return name == "[S";
    }
    [[nodiscard]] bool isByteArr() const
    {
        return name == "[B";
    }
    [[nodiscard]] bool isBooleanArr() const
    {
        return name == "[Z";
    }
    [[nodiscard]] bool isObjArr() const
    {
        return !isIntArr() && !isFloatArr() && !isLongArr() && !isDoubleArr() && !isCharArr() && !isShortArr() &&
               !isByteArr() && !isBooleanArr() && isArr();
    }
    [[nodiscard]] bool isArr() const
    {
        return name[0] == '[';
    }

    OMOOPDesc *allocateInstance()
    {
        assert(kind == OMKlassKind::Normal);
        auto obj = static_cast<OMOOPDesc *>(heap->allocate(sizeof(OMOOPDesc) + length));
        obj->klass = this;
        return obj;
    }

    OMOOPArrDesc *allocateArray(jint n)
    {
        assert(kind == OMKlassKind::Array);
        auto obj = static_cast<OMOOPArrDesc *>(heap->allocate(sizeof(OMOOPArrDesc) + length * n));
        obj->klass = this;
        obj->length = n;
        return obj;
    }
};
} // namespace openminecraft::vm::pixeltower::v0

#endif