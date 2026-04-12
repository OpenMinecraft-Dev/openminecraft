#ifndef OM_ELYSIA_KLASSLOADER_HPP
#define OM_ELYSIA_KLASSLOADER_HPP

#include "openminecraft/binary/om_bin_hash.hpp"
#include "openminecraft/log/om_log_common.hpp"
#include "openminecraft/vm/elysia/om_elysia_klass.hpp"
#include "openminecraft/vm/elysia/om_elysia_virtualworld.hpp"
#include <istream>
#include <unordered_map>
namespace openminecraft::vm::elysia
{
class OMElysiaKlassloader
{
  public:
    OMElysiaKlassloader(OMElysiaVirtualWorld *vw);
    ~OMElysiaKlassloader();

    OMElysiaInstanceKlass *constructInstanceClassShell(std::string s);
    OMElysiaPrimitiveKlass *constructPrimitiveClass(std::string s);
    OMElysiaArrayKlass *constructArrayClass(OMElysiaKlass *klass);

    void markKlass(OMElysiaKlass *klass);

    void loadClass(std::string name);
    void loadClass(std::istream *istr);

    OMElysiaKlass *findClass(std::string s);

    OMElysiaKlass *fetchOrLoadClass(std::string s)
    {
        auto l = findClass(s);
        if (l)
        {
            return l;
        }

        loadClass(s);
        return findClass(s);
    }

  private:
    void unloadClass(OMElysiaKlass *klass);

    OMElysiaVirtualWorld *world;
    std::unordered_map<binary::hash::hash_t, OMElysiaKlass *> loadedClasses;

    log::OMLogger logger;
};
} // namespace openminecraft::vm::elysia

#endif
