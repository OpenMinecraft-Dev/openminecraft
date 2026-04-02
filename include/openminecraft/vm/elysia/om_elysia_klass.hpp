#ifndef OM_ELYSIA_KLASS_HPP
#define OM_ELYSIA_KLASS_HPP

#include "openminecraft/vm/elysia/om_elysia_types.hpp"

namespace openminecraft::vm::elysia
{
enum OMElysiaKlassType
{
    InstanceKlass,
    PrimitiveKlass,
    ArrayKlass
};
class OMElysiaKlass
{
  public:
    OMElysiaKlass *superClass;
    OMElysiaKlassType type;

    jbyte *name;
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
};

class OMElysiaPrimitiveKlass : public OMElysiaKlass
{
};
} // namespace openminecraft::vm::elysia

#endif
