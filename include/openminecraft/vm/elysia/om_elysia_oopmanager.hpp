#ifndef OM_ELYSIA_OOPMANAGER
#define OM_ELYSIA_OOPMANAGER

#include "openminecraft/vm/elysia/om_elysia_klass.hpp"
#include "openminecraft/vm/elysia/om_elysia_virtualworld.hpp"
#include <cstdint>
namespace openminecraft::vm::elysia
{
#pragma pack(1)
struct OMElysiaOop
{
    int markword;
    union {
        uint32_t compressed;
        OMElysiaKlass *raw;
    };
};
#pragma pack()

class OMElysiaOopManager
{
  public:
    OMElysiaOopManager(OMElysiaVirtualWorld *world);
    ~OMElysiaOopManager();

    uint64_t oopHeaderLength();
    OMElysiaOop *allocateOop(OMElysiaKlass *klass);
    uintptr_t oopAccessField(void *base, uint64_t offset);

  private:
    OMElysiaVirtualWorld *world;
};
} // namespace openminecraft::vm::elysia

#endif
