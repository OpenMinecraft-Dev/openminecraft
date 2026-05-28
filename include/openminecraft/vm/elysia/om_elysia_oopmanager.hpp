#ifndef OM_ELYSIA_OOPMANAGER
#define OM_ELYSIA_OOPMANAGER

#include "openminecraft/log/om_log_common.hpp"
#include "openminecraft/vm/elysia/om_elysia_klass.hpp"
#include "openminecraft/vm/elysia/om_elysia_types.hpp"
#include "openminecraft/vm/elysia/om_elysia_virtualworld.hpp"
#include <cstdint>
namespace openminecraft::vm::elysia
{
constexpr int markFixed = 0x1;

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
struct OMElysiaArrayOopUncompressed : public OMElysiaOopUncompressed
{
    jint length;
};
struct OMElysiaArrayOopCompressed : public OMElysiaOopCompressed
{
    jint length;
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

    jint arrLength(OMElysiaOop *base);
    OMElysiaOop *allocateArr(OMElysiaArrayKlass *klass, jint length);

    OMElysiaOop *arrAccessPtr(OMElysiaOop *oop, jint index);
    void arrAccessPtr(OMElysiaOop *oop, jint index, OMElysiaOop *data);

    template <typename T> T *arrAccess(OMElysiaOop *oop)
    {
        return reinterpret_cast<T *>(reinterpret_cast<uintptr_t>(oop) + oopArrayHeaderLength());
    }

  private:
    OMElysiaVirtualWorld *world;
    log::OMLogger logger;
};
} // namespace openminecraft::vm::elysia

#endif
