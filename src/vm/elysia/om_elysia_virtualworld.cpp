#include "openminecraft/vm/elysia/om_elysia_virtualworld.hpp"
#include "openminecraft/log/om_log_threadname.hpp"
#include "openminecraft/mem/om_mem_stl_allocator.hpp"
#include "openminecraft/vm/elysia/executor/om_elysia_executor_zero.hpp"
#include "openminecraft/vm/elysia/impl/om_elysia_implbase.hpp"
#include "openminecraft/vm/elysia/om_elysia_klass.hpp"
#include "openminecraft/vm/elysia/om_elysia_klassloader.hpp"
#include "openminecraft/vm/elysia/om_elysia_oopmanager.hpp"
#include <stdexcept>
#include <thread>

using namespace openminecraft::vm::elysia::impl;

namespace openminecraft::vm::elysia
{
OMElysiaVirtualWorld::OMElysiaVirtualWorld()
    : metaspaceHeap("elysia_metaspace", 1024 * 1024 * 16), mainHeap("elysia_main", 1024 * 1024 * 1024),
      logger("OMElysiaVirtualWorld", this)
{
    klassLoader = mem::fast_shared<allocatorTag, OMElysiaKlassloader>(this);
    oopManager = mem::fast_shared<allocatorTag, OMElysiaOopManager>(this);
    executor = mem::fast_shared<allocatorTag, executor::OMElysiaExecutorZero>(this);

    auto klasses = {"java/lang/Object",        "java/lang/String",    "java/lang/Class",  "java/lang/Throwable",
                    "java/lang/Thread",        "java/lang/System",    "java/lang/Byte",   "java/lang/Integer",
                    "java/lang/Short",         "java/lang/Long",      "java/lang/Float",  "java/lang/Double",
                    "java/lang/Boolean",       "java/lang/Character", "java/lang/Void",   "java/lang/Runtime",
                    "java/lang/StringBuilder", "java/lang/Process",   "sun/misc/Launcher"};
    for (auto &s : klasses)
    {
        klassLoader->loadClassWithoutMirror(s);
    }

    auto clsobj = klassLoader->findClass("java/lang/Object");

    for (auto &s : {"char", "byte", "short", "int", "long", "float", "double", "boolean"})
    {
        auto c = klassLoader->constructPrimitiveClass(s);
        auto carr = klassLoader->constructArrayClass(c);
        carr->superClass = clsobj;
    }

    klassLoader->constructPrimitiveClass("void");
    klassLoader->fixAllClasses();

    registerNative(Java_java_lang_System_registerNatives);
    registerNative(Java_java_lang_Object_registerNatives);
    registerNative(Java_java_lang_Class_registerNatives);
    registerNative(Java_java_lang_Float_floatToRawIntBits);
    registerNative(Java_java_lang_Double_longBitsToDouble);
    registerNative(Java_java_lang_Double_doubleToRawLongBits);
    registerNative(Java_sun_misc_VM_initialize);
    registerNative(Java_java_io_FileDescriptor_initIDs);

    auto tt = new std::thread([&]() {
        log::multithread::registerCurrentThreadName("main");
        try
        {
            auto mcls = klassLoader->findClass("java/lang/System");
            auto md = mcls->findMethod("initializeSystemClass", "()V");
            executor->callVoidFunction(md);
        }
        catch (std::logic_error &e)
        {
            logger.error("Elysia VM throwed an exception: {}", e.what());
            while (true)
            {
	    std::this_thread::sleep_for(std::chrono::milliseconds(1000));
            }
        }
    });
}
OMElysiaVirtualWorld::~OMElysiaVirtualWorld()
{
}
} // namespace openminecraft::vm::elysia
