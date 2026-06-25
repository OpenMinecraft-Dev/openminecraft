#include "openminecraft/vm/elysia/interface/om_elysia_interface_defs.hpp"
#include "openminecraft/vm/elysia/om_elysium.hpp"
#include <windows.h>

namespace openminecraft::vm::elysia
{
void Java_java_io_WinNTFileSystem_initIDs(OMElysiaJNIEnv *env, OMElysiaKlass *)
{
}

jlong Java_sun_io_Win32ErrorMode_setErrorMode(OMElysiaJNIEnv *env, OMElysiaKlass *cls, jlong mode)
{
    UINT uMode = (UINT)mode;
    UINT oldMode = SetErrorMode(uMode);
    return (jlong)oldMode;
}

void OMElysium::registerPlatformNative()
{
    registerNative(Java_java_io_WinNTFileSystem_initIDs);
    registerNative(Java_sun_io_Win32ErrorMode_setErrorMode);
}
} // namespace openminecraft::vm::elysia
