#ifndef OM_ELYSIA_KLASSLOADER_HPP
#define OM_ELYSIA_KLASSLOADER_HPP

#include "openminecraft/binary/om_bin_hash.hpp"
#include "openminecraft/log/om_log_common.hpp"
#include "openminecraft/vm/elysia/om_elysia_klass.hpp"
#include "openminecraft/vm/elysia/om_elysia_oopmanager.hpp"
#include "openminecraft/vm/elysia/om_elysium.hpp"
#include <istream>
#include <map>
#include <memory>
namespace openminecraft::vm::elysia
{
class OMElysiaKlassloader
{
  public:
    OMElysiaKlassloader(OMElysium *vw);
    ~OMElysiaKlassloader();

    auto constructInstanceClassShell(std::string s) -> OMElysiaInstanceKlass *;
    auto constructPrimitiveClass(std::string s) -> OMElysiaPrimitiveKlass *;
    auto constructArrayClass(OMElysiaKlass *klass) -> OMElysiaArrayKlass *;

    void markKlass(OMElysiaKlass *klass);

    void fillVtable(OMElysiaInstanceKlass *klass);
    auto loadClassWithoutMirror(std::string name, bool special = false) -> OMElysiaKlass *;
    auto loadClassWithoutMirror(std::shared_ptr<std::istream> istr, bool special = false, std::string repname = "")
        -> OMElysiaKlass *;
    void fixClassMirror(OMElysiaKlass *klass);
    void fixAllClasses();

    auto findClass(std::string s) -> OMElysiaKlass *;
    inline auto fetchOrLoadClass(std::string s, bool needInit = false) -> OMElysiaKlass *
    {
        auto l = findClass(s);
        if (!l)
        {
            loadClassWithoutMirror(s);
            l = findClass(s);
        }

        if (l)
        {
            fixClassMirror(l);
            if (needInit)
            {
                ensureClassInit(l);
            }
        }

        return l;
    }
    void ensureClassInit(OMElysiaKlass *);

    inline auto upper() -> OMElysium *
    {
        return elysium;
    }

    OMElysiaOop *klassloader = nullptr;
    std::shared_ptr<OMElysiaKlassloader> next = nullptr;

    std::shared_ptr<std::map<binary::hash::hash_t, OMElysiaKlass *>> loadedClasses;

    OMElysium *elysium;

  private:
    log::OMLogger logger;
};
} // namespace openminecraft::vm::elysia

#endif
