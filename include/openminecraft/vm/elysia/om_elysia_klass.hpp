#ifndef OM_ELYSIA_KLASS_HPP
#define OM_ELYSIA_KLASS_HPP

#include "openminecraft/vm/classfile/om_class_file.hpp"
#include "openminecraft/vm/elysia/interface/om_elysia_interface_defs.hpp"
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

class OMElysiaInstanceKlass;
class OMElysiaPrimitiveKlass;
class OMElysiaArrayKlass;

enum OMElysiaKlassType
{
    InstanceKlass,
    PrimitiveKlass,
    ArrayKlass
};
class OMElysiaKlass
{
  public:
    OMElysiaKlassloader *klassloader = nullptr;

    OMElysiaKlass *superClass;
    OMElysiaKlassType type;
    uint8_t ptrLength;

    uint16_t accessFlag;

    char *name;

    OMElysiaOop *mirror = nullptr;

    uint32_t methodCount = 0;
    OMElysiaMethod *methods = nullptr;
    uint32_t vtableLength = 0;
    OMElysiaMethod **vtable = nullptr;

    uint32_t nativeMethodCount = 0;
    OMElysiaNativeMethod *nativeMethods = nullptr;

    OMElysiaMethod *findMethod(const char *name, const char *desc);

    OMElysiaInstanceKlass *toInstance()
    {
        return reinterpret_cast<OMElysiaInstanceKlass *>(this);
    }

    bool isInstance()
    {
        return type == InstanceKlass;
    }

    OMElysiaPrimitiveKlass *toPrimitive()
    {
        return reinterpret_cast<OMElysiaPrimitiveKlass *>(this);
    }

    bool isPrimitive()
    {
        return type == PrimitiveKlass;
    }

    OMElysiaArrayKlass *toArray()
    {
        return reinterpret_cast<OMElysiaArrayKlass *>(this);
    }

    bool isArray()
    {
        return type == ArrayKlass;
    }

    bool inherits(OMElysiaKlass *);
};

class OMElysiaArrayKlass : public OMElysiaKlass
{
  public:
    OMElysiaKlass *lowerDim;
    OMElysiaKlass *higherDim;

    uint32_t itemLength;
};

class OMElysiaInstanceKlass : public OMElysiaKlass
{
  public:
    uint32_t interfaceImplCount;
    OMElysiaKlass **interfaceImpls;

    std::shared_ptr<std::unordered_map<uint16_t, std::shared_ptr<classfile::OMClassConstant>>> constantPoolRaw;
    uint32_t constantPoolCount = 0;
    void **constantPool = nullptr;
    bool *constantPoolState = nullptr;

    bool clinitFinished = false;

    uint32_t fieldCount = 0;
    OMElysiaField *fields = nullptr;
    bool fieldOffsetInited = false;

    uint32_t length = 0;
    uint32_t staticLength = 0;

    void *staticBlock = nullptr;

    void initFieldOffsets();

    OMElysiaField *findField(const char *name, const char *desc);

    void *constantPoolFetchField(uint16_t id);
    void *constantPoolFetchNormal(uint16_t id, bool flg = false);
    uint64_t constantPoolFetchNormalW(uint16_t id);
};

class OMElysiaPrimitiveKlass : public OMElysiaKlass
{
};
} // namespace openminecraft::vm::elysia

#endif
