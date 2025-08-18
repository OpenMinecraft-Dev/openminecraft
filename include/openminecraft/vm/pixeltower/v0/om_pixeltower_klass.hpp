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
    std::unordered_map<std::string, OMMethod *> *vtable;
    AccessFlags accessFlags;
    std::vector<OMField> fields;
    uint64_t length;
    void *staticBlock;
    uint64_t staticLength;

    void **constantPool;

    OMPixelTowerHeap *heap;

    inline bool isIntArr()
    {
        return name == "[I";
    }
    inline bool isFloatArr()
    {
        return name == "[F";
    }
    inline bool isLongArr()
    {
        return name == "[J";
    }
    inline bool isDoubleArr()
    {
        return name == "[D";
    }
    inline bool isCharArr()
    {
        return name == "[C";
    }
    inline bool isShortArr()
    {
        return name == "[S";
    }
    inline bool isByteArr()
    {
        return name == "[B";
    }
    inline bool isBooleanArr()
    {
        return name == "[Z";
    }
    inline bool isObjArr()
    {
        return !isIntArr() && !isFloatArr() && !isLongArr() && !isDoubleArr() && !isCharArr() && !isShortArr() &&
               !isByteArr() && !isBooleanArr() && isArr();
    }
    inline bool isArr()
    {
        return name[0] == '[';
    }

    inline OMOOPDesc *allocateInstance()
    {
        assert(kind == OMKlassKind::Normal);
        auto obj = (OMOOPDesc *)heap->allocate(sizeof(OMOOPDesc) + length);
        obj->klass = this;
        return obj;
    }

    inline OMOOPArrDesc *allocateArray(jint n)
    {
        assert(kind == OMKlassKind::Array);
        auto obj = (OMOOPArrDesc *)heap->allocate(sizeof(OMOOPArrDesc) + length * n);
        obj->klass = this;
        obj->length = n;
        return obj;
    }
};
} // namespace openminecraft::vm::pixeltower::v0

#endif