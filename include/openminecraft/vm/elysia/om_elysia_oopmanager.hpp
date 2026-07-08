#ifndef OM_ELYSIA_OOPMANAGER
#define OM_ELYSIA_OOPMANAGER

#include "openminecraft/log/om_log_common.hpp"
#include "openminecraft/vm/elysia/om_elysia_klass.hpp"
#include "openminecraft/vm/elysia/om_elysia_types.hpp"
#include "openminecraft/vm/elysia/om_elysium.hpp"
#include <cstdint>
#include <random>
namespace openminecraft::vm::elysia
{
constexpr int markFixed = 0x80000000;

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
    OMElysiaOopManager(OMElysium *elysium);
    ~OMElysiaOopManager();

    auto oopHeaderLength() -> uint64_t;
    auto oopArrayHeaderLength() -> uint64_t;
    auto allocateOop(OMElysiaKlass *klass, uint64_t extraLength = 0) -> OMElysiaOop *;
    auto allocateString(std::string target) -> OMElysiaOop *;
    auto oopGetKlass(OMElysiaOop *base) -> OMElysiaKlass *;
    auto oopAccessField(OMElysiaOop *base, uint64_t offset) -> uintptr_t;
    void oopAccessPointerField(OMElysiaOop *base, uint64_t offset, void *ptrToWrite);
    auto oopAccessPointerStaticField(OMElysiaKlass *kls, uint64_t offset) -> OMElysiaOop *;
    void oopAccessPointerStaticField(OMElysiaKlass *kls, uint64_t offset, void *ptrToWrite);
    auto oopAccessPointerField(OMElysiaOop *base, uint64_t offset) -> OMElysiaOop *;

    auto arrLength(OMElysiaOop *base) -> jint;
    auto allocateArr(OMElysiaArrayKlass *klass, jint length) -> OMElysiaOop *;
    auto allocateMultiArr(OMElysiaArrayKlass *klass, jint dim, jint *lengths) -> OMElysiaOop *;

    auto arrAccessPtr(OMElysiaOop *oop, jint index) -> OMElysiaOop *;
    void arrAccessPtr(OMElysiaOop *oop, jint index, OMElysiaOop *data);

    template <typename T> inline auto arrAccess(OMElysiaOop *oop) -> T *
    {
        return reinterpret_cast<T *>(reinterpret_cast<uintptr_t>(oop) + oopArrayHeaderLength());
    }

    auto oopLength(OMElysiaOop *oop) -> uint64_t;

  private:
    OMElysium *elysium;
    log::OMLogger logger;

    std::mt19937 generator;
    std::uniform_int_distribution<int> dis;
};
} // namespace openminecraft::vm::elysia

#endif
