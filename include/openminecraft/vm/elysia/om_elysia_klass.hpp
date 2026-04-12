#ifndef OM_ELYSIA_KLASS_HPP
#define OM_ELYSIA_KLASS_HPP

#include "openminecraft/vm/classfile/om_class_file.hpp"
#include "openminecraft/vm/elysia/om_elysia_field.hpp"
#include "openminecraft/vm/elysia/om_elysia_method.hpp"
#include "openminecraft/vm/elysia/om_elysia_types.hpp"
#include <cstdint>
#include <memory>
#include <unordered_map>

namespace openminecraft::vm::elysia
{
class OMElysiaOop;
class OMElysiaKlassloader;

enum OMElysiaKlassType
{
    InstanceKlass,
    PrimitiveKlass,
    ArrayKlass
};
class OMElysiaKlass
{
  public:
    OMElysiaKlassloader *nativeKlassloader = nullptr;

    OMElysiaKlass *superClass;
    OMElysiaKlassType type;
    uint8_t ptrLength;

    uint16_t accessFlag;

    jbyte *name;

    OMElysiaOop *klassloader = nullptr;

    uint32_t methodCount = 0;
    OMElysiaMethod *methods = nullptr;

    OMElysiaMethod *findMethod(const char *name, const char *desc);
};

class OMElysiaArrayKlass : public OMElysiaKlass
{
  public:
    OMElysiaKlass *lowerDim;
    OMElysiaKlass *higherDim;
};

class OMElysiaInstanceKlass : public OMElysiaKlass
{
  public:
    uint32_t interfaceImplCount;
    OMElysiaKlass **interfaceImpls;

    std::unordered_map<uint16_t, std::shared_ptr<classfile::OMClassConstant>> constantPoolRaw;
    uint32_t constantPoolCount = 0;
    void **constantPool = nullptr;

    bool clinitFinished = false;

    uint32_t fieldCount = 0;
    OMElysiaField *fields = nullptr;
    bool fieldOffsetInited = false;

    uint32_t length = 0;
    uint32_t staticLength = 0;

    void *staticBlock = nullptr;

    void initFieldOffsets();

    OMElysiaField *findField(const char *name, const char *desc);
    void *constantPoolFetch(uint16_t id);
};

class OMElysiaPrimitiveKlass : public OMElysiaKlass
{
};
} // namespace openminecraft::vm::elysia

#endif
