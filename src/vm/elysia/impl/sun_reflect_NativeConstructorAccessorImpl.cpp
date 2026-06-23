#include "openminecraft/vm/elysia/impl/om_elysia_implbase.hpp"
#include "openminecraft/vm/elysia/om_elysia_klass.hpp"
#include <stdexcept>

namespace openminecraft::vm::elysia::impl
{
extern "C"
{
    OMElysiaNativeHandle *Java_sun_reflect_NativeConstructorAccessorImpl_newInstance0(OMElysiaJNIEnv *env,
                                                                                      OMElysiaKlass *,
                                                                                      OMElysiaNativeHandle *constructor,
                                                                                      OMElysiaNativeHandle *args)
    {
        throw std::logic_error("not impl!");
    }
}
} // namespace openminecraft::vm::elysia::impl
