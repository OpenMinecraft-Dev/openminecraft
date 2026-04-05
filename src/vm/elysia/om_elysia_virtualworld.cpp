#include "openminecraft/vm/elysia/om_elysia_virtualworld.hpp"
#include "openminecraft/mem/om_mem_stl_allocator.hpp"
#include "openminecraft/vm/elysia/executor/om_elysia_executor_zero.hpp"
#include "openminecraft/vm/elysia/om_elysia_klass.hpp"
#include "openminecraft/vm/elysia/om_elysia_klassloader.hpp"
#include "openminecraft/vm/elysia/om_elysia_oopmanager.hpp"
#include <fstream>

namespace openminecraft::vm::elysia
{
OMElysiaVirtualWorld::OMElysiaVirtualWorld()
    : metaspaceHeap("elysia_metaspace", 1024 * 1024 * 16), mainHeap("elysia_main", 1024 * 1024 * 1024),
      logger("OMElysiaVirtualWorld", this)
{
    klassLoader = mem::fast_shared<allocatorTag, OMElysiaKlassloader>(this);
    oopManager = mem::fast_shared<allocatorTag, OMElysiaOopManager>(this);
    executor = mem::fast_shared<allocatorTag, executor::OMElysiaExecutorZero>(this);
    // TODO: Stage #0 basic & primitive class defines
    auto clsobj = klassLoader->constructInstanceClassShell("java/lang/Object");
    auto clsstr = klassLoader->constructInstanceClassShell("java/lang/String");
    clsstr->superClass = clsobj;
    auto clscls = klassLoader->constructInstanceClassShell("java/lang/Class");
    clscls->superClass = clsobj;

    for (auto &s : {"char", "byte", "short", "int", "long", "float", "double", "boolean"})
    {
        auto c = klassLoader->constructPrimitiveClass(s);
        auto carr = klassLoader->constructArrayClass(c);
        carr->superClass = clsobj;
    }

    klassLoader->constructPrimitiveClass("void");

    // TODO: Stage #1 load classfiles (klassloader)
    klassLoader->initClass(clsobj);
    klassLoader->initClass(clsstr);
    klassLoader->initClass(clscls);

    std::ifstream iss("/bridge/projects/Test.class", std::ios::binary);
    klassLoader->loadClass(&iss);

    auto mcls = klassLoader->findClass("openminecraft/Test");
    auto md = mcls->findMethod("main", "[Ljava/lang/String;");
    executor->execute(md);
}
OMElysiaVirtualWorld::~OMElysiaVirtualWorld()
{
}
} // namespace openminecraft::vm::elysia
