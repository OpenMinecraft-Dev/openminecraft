#ifndef OM_ELYSIA_KLASSLOADER_HPP
#define OM_ELYSIA_KLASSLOADER_HPP

#include "openminecraft/binary/om_bin_hash.hpp"
#include "openminecraft/log/om_log_common.hpp"
#include "openminecraft/vm/elysia/om_elysia_klass.hpp"
#include "openminecraft/vm/elysia/om_elysia_oopmanager.hpp"
#include "openminecraft/vm/elysia/om_elysium.hpp"
#include <istream>
#include <map>
namespace openminecraft::vm::elysia
{
class OMElysiaKlassloader
{
  public:
    OMElysiaKlassloader(OMElysium *vw);
    ~OMElysiaKlassloader();

    OMElysiaInstanceKlass *constructInstanceClassShell(std::string s);
    OMElysiaPrimitiveKlass *constructPrimitiveClass(std::string s);
    OMElysiaArrayKlass *constructArrayClass(OMElysiaKlass *klass);

    void markKlass(OMElysiaKlass *klass);

    void loadClassWithoutMirror(std::string name, bool special = false);
    void loadClassWithoutMirror(std::istream *istr, bool special = false);
    void fixClassMirror(OMElysiaKlass *klass);
    void fixAllClasses();

    OMElysiaKlass *findClass(std::string s);

    OMElysiaKlass *fetchOrLoadClass(std::string s)
    {
        auto l = findClass(s);
        if (l)
        {
            fixClassMirror(l);
            return l;
        }

        loadClassWithoutMirror(s);
        auto ll = findClass(s);
        fixClassMirror(ll);
        return ll;
    }

    OMElysium *upper()
    {
        return elysium;
    }

    OMElysiaOop *klassloader = nullptr;
    std::shared_ptr<OMElysiaKlassloader> next = nullptr;

  private:
    OMElysium *elysium;
    std::shared_ptr<std::map<binary::hash::hash_t, OMElysiaKlass *>> loadedClasses;

    log::OMLogger logger;
};
} // namespace openminecraft::vm::elysia

#endif
