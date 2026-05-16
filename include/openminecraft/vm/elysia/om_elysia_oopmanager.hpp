#ifndef OM_ELYSIA_OOPMANAGER
#define OM_ELYSIA_OOPMANAGER

#include "openminecraft/log/om_log_common.hpp"
#include "openminecraft/vm/elysia/om_elysia_klass.hpp"
#include "openminecraft/vm/elysia/om_elysia_types.hpp"
#include "openminecraft/vm/elysia/om_elysia_virtualworld.hpp"
#include <cstdint>
namespace openminecraft::vm::elysia
{
constexpr int markEden = 0x1;

#pragma pack(1)
struct OMElysiaOop
{
    int markword;
};
struct OMElysiaOopUncompressed : public OMElysiaOop
{
    OMElysiaKlass *klass;
};
struct OMElysiaOopCompressed : public OMElysiaOop
{
    uint32_t klass;
};
struct OMElysiaArrayOop : public OMElysiaOop
{
    jint length;
};
struct OMElysiaArrayOopUncompressed : public OMElysiaArrayOop
{
    OMElysiaKlass *klass;
};
struct OMElysiaArrayOopCompressed : public OMElysiaArrayOop
{
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
    OMElysiaOop *allocateString(std::string &target);
    OMElysiaKlass *oopGetKlass(OMElysiaOop *base);
    uintptr_t oopAccessField(OMElysiaOop *base, uint64_t offset);
    void oopAccessPointerField(OMElysiaOop *base, uint64_t offset, void *ptrToWrite);
    OMElysiaOop *oopAccessPointerField(OMElysiaOop *base, uint64_t offset);

    OMElysiaArrayOop *allocateArr(OMElysiaArrayKlass *klass, jint length);

    template <typename T> T *arrAccess(OMElysiaArrayOop *oop)
    {
        return reinterpret_cast<T *>(reinterpret_cast<uintptr_t>(oop) + oopArrayHeaderLength());
    }

  private:
    OMElysiaVirtualWorld *world;
    log::OMLogger logger;
};
} // namespace openminecraft::vm::elysia

#endif
