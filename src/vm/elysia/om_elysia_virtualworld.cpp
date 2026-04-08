#include "openminecraft/vm/elysia/om_elysia_virtualworld.hpp"
#include "openminecraft/mem/om_mem_stl_allocator.hpp"
#include "openminecraft/vm/elysia/executor/om_elysia_executor_zero.hpp"
#include "openminecraft/vm/elysia/om_elysia_klass.hpp"
#include "openminecraft/vm/elysia/om_elysia_klassloader.hpp"
#include "openminecraft/vm/elysia/om_elysia_oopmanager.hpp"
#include <fstream>
#include <thread>

namespace openminecraft::vm::elysia
{
OMElysiaVirtualWorld::OMElysiaVirtualWorld()
    : metaspaceHeap("elysia_metaspace", 1024 * 1024 * 16), mainHeap("elysia_main", 1024 * 1024 * 1024),
      logger("OMElysiaVirtualWorld", this)
{
    klassLoader = mem::fast_shared<allocatorTag, OMElysiaKlassloader>(this);
    oopManager = mem::fast_shared<allocatorTag, OMElysiaOopManager>(this);
    executor = mem::fast_shared<allocatorTag, executor::OMElysiaExecutorZero>(this);

    for (auto &s : {
        "java/lang/Object",
	"java/lang/String",
	"java/lang/Class",
	"java/lang/Throwable",
	"java/lang/Thread",
	"java/lang/System",
	"java/lang/Byte",
	"java/lang/Integer",
	"java/lang/Short",
	"java/lang/Long",
	"java/lang/Float",
	"java/lang/Double",
	"java/lang/Boolean",
	"java/lang/Character",
	"java/lang/Void",
	"java/lang/Runtime",
	"java/lang/StringBuilder",
	"java/lang/Process"
    }) {
        klassLoader->loadClass(s);
    }

    auto clsobj = klassLoader->findClass("java/lang/Object");

    for (auto &s : {"char", "byte", "short", "int", "long", "float", "double", "boolean"})
    {
        auto c = klassLoader->constructPrimitiveClass(s);
        auto carr = klassLoader->constructArrayClass(c);
        carr->superClass = clsobj;
    }

    klassLoader->constructPrimitiveClass("void");

    std::ifstream iss("../Test.class", std::ios::binary);
    klassLoader->loadClass(&iss);

    auto tt = new std::thread([&]() {
        auto mcls = klassLoader->findClass("openminecraft/Test");
        auto md = mcls->findMethod("main", "[Ljava/lang/String;");
        executor->execute(md);
    });
}
OMElysiaVirtualWorld::~OMElysiaVirtualWorld()
{
}
} // namespace openminecraft::vm::elysia
