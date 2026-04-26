#ifndef OM_ELYSIA_OOPMANAGER
#define OM_ELYSIA_OOPMANAGER

#include "openminecraft/vm/elysia/om_elysia_klass.hpp"
#include "openminecraft/vm/elysia/om_elysia_types.hpp"
#include "openminecraft/vm/elysia/om_elysia_virtualworld.hpp"
#include <cstdint>
namespace openminecraft::vm::elysia
{
#pragma pack(1)
struct OMElysiaOop
{
    int markword;
};
struct OMElysiaOopUncompressed
{
    int markword;
    OMElysiaKlass *klass;
};
struct OMElysiaOopCompressed
{
    int markword;
    uint32_t klass;
};
struct OMElysiaArrayOop
{
    int markword;
    jint length;
};
struct OMElysiaArrayOopUncompressed
{
    int markword;
    jint length;
    OMElysiaKlass *klass;
};
struct OMElysiaArrayOopCompressed
{
    int markword;
    jint length;
    uint32_t klass;
};
#pragma pack()

class OMElysiaOopManager
{
  public:
    OMElysiaOopManager(OMElysiaVirtualWorld *world);
    ~OMElysiaOopManager();

    uint64_t oopHeaderLength();
    uint64_t oopArrayHeaderLength();
    OMElysiaOop *allocateOop(OMElysiaKlass *klass);
    uintptr_t oopAccessField(void *base, uint64_t offset);

    OMElysiaArrayOop *allocateArr(OMElysiaArrayKlass *klass, jint length);

    template <typename T> T *arrAccess(OMElysiaArrayOop *oop)
    {
        return reinterpret_cast<T *>(reinterpret_cast<uintptr_t>(oop) + oopArrayHeaderLength());
    }

  private:
    OMElysiaVirtualWorld *world;
};
} // namespace openminecraft::vm::elysia

#endif
