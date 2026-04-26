#ifndef OM_ELYSIA_IMPLBASE_HPP
#define OM_ELYSIA_IMPLBASE_HPP

namespace openminecraft::vm::elysia
{
class OMElysiaVirtualWorld;
};

namespace openminecraft::vm::elysia::impl
{
void Java_java_lang_System_registerNatives(OMElysiaVirtualWorld *world);
};

#endif
