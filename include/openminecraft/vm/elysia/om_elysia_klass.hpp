#ifndef OM_ELYSIA_KLASS_HPP
#define OM_ELYSIA_KLASS_HPP

#include "openminecraft/specs/classfile/om_classfile.hpp"
#include "openminecraft/vm/elysia/interface/om_elysia_interface_defs.hpp"
#include "openminecraft/vm/elysia/om_elysia_field.hpp"
#include "openminecraft/vm/elysia/om_elysia_method.hpp"
#include <cstdint>
#include <memory>
#include <mutex>

namespace openminecraft::vm::elysia
{
struct OMElysiaOop;
class OMElysiaKlassloader;

class OMElysiaInstanceKlass;
class OMElysiaPrimitiveKlass;
class OMElysiaArrayKlass;

struct OMElysiaKlassDynamic
{
    OMElysiaMethod *target;
    OMElysiaOop *handle;
};

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
    uint16_t thisClass;

    char *name;

    OMElysiaOop *mirror = nullptr;

    uint32_t methodCount = 0;
    OMElysiaMethod *methods = nullptr;
    uint32_t vtableLength = 0;
    OMElysiaMethod **vtable = nullptr;

    uint32_t nativeMethodCount = 0;
    OMElysiaNativeMethod *nativeMethods = nullptr;

    uint32_t bootstrapMethodCount = 0;
    specs::classfile::OMClassBootstrapMethodEntry *bootstrapMethods = nullptr;

    std::unique_ptr<std::recursive_mutex> klassMutex;

    auto findMethod(const char *name, const char *desc) -> OMElysiaMethod *;

    inline auto toInstance() -> OMElysiaInstanceKlass *
    {
        return reinterpret_cast<OMElysiaInstanceKlass *>(this);
    }

    inline auto isInstance() -> bool
    {
        return type == InstanceKlass;
    }

    inline auto toPrimitive() -> OMElysiaPrimitiveKlass *
    {
        return reinterpret_cast<OMElysiaPrimitiveKlass *>(this);
    }

    inline auto isPrimitive() -> bool
    {
        return type == PrimitiveKlass;
    }

    inline auto toArray() -> OMElysiaArrayKlass *
    {
        return reinterpret_cast<OMElysiaArrayKlass *>(this);
    }

    inline auto isArray() -> bool
    {
        return type == ArrayKlass;
    }

    auto inherits(OMElysiaKlass *) -> bool;
};

class OMElysiaInstanceKlass : public OMElysiaKlass
{
  public:
    uint32_t interfaceImplCount;
    OMElysiaKlass **interfaceImpls;

    std::vector<specs::classfile::OMClassFileConstant> constantPoolRaw;
    uint32_t constantPoolCount = 0;
    void **constantPool = nullptr;
    bool *constantPoolState = nullptr;

    bool clinitFinished = false;

    OMElysiaKlass *enclosingKlass = nullptr;
    OMElysiaMethod *enclosingMethod = nullptr;

    uint32_t fieldCount = 0;
    OMElysiaField *fields = nullptr;
    bool fieldOffsetInited = false;

    uint32_t length = 0;
    uint32_t staticLength = 0;

    void *staticBlock = nullptr;

    void initFieldOffsets();

    auto findField(const char *name, const char *desc) -> OMElysiaField *;

    auto constantPoolFetchField(uint16_t id) -> void *;
    auto constantPoolFetchNormal(uint16_t id, bool flg = false) -> void *;
    auto constantPoolFetchDynamic(uint16_t id) -> void *;
    auto constantPoolFetchNormalW(uint16_t id) -> uint64_t;
};

class OMElysiaArrayKlass : public OMElysiaInstanceKlass
{
  public:
    OMElysiaKlass *lowerDim;
    OMElysiaKlass *higherDim;

    uint32_t itemLength;
};

class OMElysiaPrimitiveKlass : public OMElysiaKlass
{
};
} // namespace openminecraft::vm::elysia

#endif
