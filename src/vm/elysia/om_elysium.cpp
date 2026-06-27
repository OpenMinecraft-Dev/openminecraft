#include "openminecraft/vm/elysia/om_elysium.hpp"
#include "openminecraft/log/om_log_threadname.hpp"
#include "openminecraft/mem/om_mem_stl_allocator.hpp"
#include "openminecraft/vm/elysia/executor/om_elysia_executor_zero.hpp"
#include "openminecraft/vm/elysia/impl/om_elysia_implbase.hpp"
#include "openminecraft/vm/elysia/interface/om_elysia_interface_defs.hpp"
#include "openminecraft/vm/elysia/om_elysia_klass.hpp"
#include "openminecraft/vm/elysia/om_elysia_klassloader.hpp"
#include "openminecraft/vm/elysia/om_elysia_monitormanager.hpp"
#include "openminecraft/vm/elysia/om_elysia_oopmanager.hpp"
#include "openminecraft/vm/elysia/om_elysia_threadmodel.hpp"
#include <atomic>
#include <stdexcept>
#include <thread>

using namespace openminecraft::vm::elysia::impl;

std::atomic<bool> needStop = false;

namespace openminecraft::vm::elysia
{
OMElysium::OMElysium()
    : metaspaceHeap("elysia_metaspace", 1024 * 1024 * 16, 0.2), mainHeap("elysia_main", 1024 * 1024 * 1024, 0.25),
      logger("OMElysium", this)
{
    monitorManager = mem::fast_shared<allocatorTag, OMElysiaMonitorManager>();
    klassLoader = mem::fast_shared<allocatorTag, OMElysiaKlassloader>(this);
    oopManager = mem::fast_shared<allocatorTag, OMElysiaOopManager>(this);
    executor = mem::fast_shared<allocatorTag, executor::OMElysiaExecutorZero>(this);

    registerNative(Java_java_lang_System_registerNatives);
    registerNative(Java_java_lang_Object_registerNatives);
    registerNative(Java_java_lang_Class_registerNatives);
    registerNative(Java_java_lang_Thread_registerNatives);
    registerNative(Java_java_lang_Float_floatToRawIntBits);
    registerNative(Java_java_lang_Double_longBitsToDouble);
    registerNative(Java_java_lang_Double_doubleToRawLongBits);
    registerNative(Java_sun_misc_VM_initialize);
    registerNative(Java_java_io_FileDescriptor_initIDs);
    registerNative(Java_java_io_FileInputStream_initIDs);
    registerNative(Java_sun_misc_Unsafe_registerNatives);
    registerNative(Java_sun_reflect_Reflection_getCallerClass);
    registerNative(Java_java_io_FileOutputStream_initIDs);
    registerNative(Java_java_security_AccessController_doPrivileged);
    registerNative(Java_java_security_AccessController_getStackAccessControlContext);
    registerNative(Java_java_lang_ClassLoader_registerNatives);
    registerNative(Java_java_lang_String_intern);
    registerNative(Java_sun_reflect_Reflection_getClassAccessFlags);
    registerNative(Java_java_lang_Throwable_fillInStackTrace);
    registerNative(Java_sun_reflect_NativeConstructorAccessorImpl_newInstance0);
    registerNative(Java_java_util_concurrent_atomic_AtomicLong_VMSupportsCS8);
    registerNative(Java_java_lang_ClassLoader$NativeLibrary_load);
    registerNative(Java_sun_misc_Signal_findSignal);
    registerNative(Java_sun_misc_Signal_handle0);
    registerNative(Java_java_lang_Runtime_availableProcessors);
    registerNative(Java_sun_misc_URLClassPath_getLookupCacheURLs);
    registerNative(Java_java_lang_Float_intBitsToFloat);
    registerPlatformNative();

    mainThread = new std::thread([&]() {
        log::multithread::registerCurrentThreadName("main");
        thisThread.metadata->threadName = "main";
        try
        {
            auto prim = {"char", "byte", "short", "int", "long", "float", "double", "boolean"};

            for (auto &s : prim)
            {
                auto c = klassLoader->constructPrimitiveClass(s);
            }

            auto klasses = {
                "java/lang/Object",        "java/lang/String",    "java/lang/Class",      "java/lang/Throwable",
                "java/lang/Thread",        "java/lang/System",    "java/lang/Byte",       "java/lang/Integer",
                "java/lang/Short",         "java/lang/Long",      "java/lang/Float",      "java/lang/Double",
                "java/lang/Boolean",       "java/lang/Character", "java/lang/Void",       "java/lang/Runtime",
                "java/lang/StringBuilder", "java/lang/Process",   "java/lang/ThreadGroup"};
            for (auto &s : klasses)
            {
                klassLoader->loadClassWithoutMirror(s, true);
            }
            auto clsobj = klassLoader->findClass("java/lang/Object");
            for (auto &s : prim)
            {
                auto carr = klassLoader->constructArrayClass(klassLoader->findClass(s));
                carr->superClass = clsobj;
            }
            klassLoader->constructPrimitiveClass("void");
            klassLoader->fixAllClasses();

            auto mcls = klassLoader->findClass("java/lang/System");
            auto md = mcls->findMethod("initializeSystemClass", "()V");
            executor->callVoidFunction(md, nullptr);
            logger.info("vm init finished");

            if (thisThread.metadata->haveException)
            {
                auto env = thisThread.metadata->interface;
                auto l = env.GetFieldID(env.FindClass("java/lang/Throwable"), "detailMessage", "Ljava/lang/String;");

                auto str = env.GetObjectField(createTempHandle(thisThread.metadata->currentException), l);

                logger.error("{}", env.GetStringUTFChars(str, nullptr));
            }

            auto l = klassLoader->fetchOrLoadClass("dev/openminecraft/MainKt");
            auto mm = l->findMethod("main", "([Ljava/lang/String;)V");
            OMElysiaNativeValue vv[1];
            vv[0].l = nullptr;
            executor->callVoidFunction(mm, vv);

            logger.info("main func ended");

            if (thisThread.metadata->haveException)
            {
                auto env = thisThread.metadata->interface;
                auto l = env.GetFieldID(env.FindClass("java/lang/Throwable"), "detailMessage", "Ljava/lang/String;");

                auto str = env.GetObjectField(createTempHandle(thisThread.metadata->currentException), l);

                logger.error("{}", env.GetStringUTFChars(str, nullptr));
            }
        }

        catch (std::logic_error &e)
        {
            logger.error("Elysia VM throwed an exception: {}", e.what());
            while (true && !needStop)
            {
                std::this_thread::sleep_for(std::chrono::milliseconds(1000));
            }
        }
    });
}
OMElysium::~OMElysium()
{
    needStop = true;
    mainThread->join();
    delete mainThread;
}

void OMElysium::startThread(OMElysiaNativeHandle *thread)
{
    auto tc = thisThread.metadata;
    auto thrcls = tc->interface.FindClass("java/lang/Thread");
    auto thrmthd = tc->interface.GetMethodID(thrcls, "run", "()V");
    tc->interface.SetIntField(thread, tc->interface.GetFieldID(thrcls, "threadStatus", "I"), 1);

    auto thr = std::make_shared<std::thread>([thread, thrcls, thrmthd, this]() {
        OMElysiaNativeValue values[1];
        values[0].l = thread;
        this->executor->callVoidFunction(thrmthd, values);

        auto tc = thisThread.metadata;
        tc->interface.SetIntField(thread, tc->interface.GetFieldID(thrcls, "threadStatus", "I"), 5);
    });
    threads.push_back(thr);
}

void OMElysium::setupThreadObject()
{
    auto tc = thisThread.metadata;
    if (!threadObjects.systemGroup)
    {
        auto tg = tc->interface.FindClass("java/lang/ThreadGroup");
        auto mthd = tc->interface.GetMethodID(tg, "<init>", "()V");

        auto obj = tc->interface.NewObjectA(tg, mthd, nullptr);
        threadObjects.systemGroup = obj;
    }

    if (!threadObjects.mainGroup)
    {
        auto tg = tc->interface.FindClass("java/lang/ThreadGroup");
        auto mthd = tc->interface.GetMethodID(tg, "<init>", "(Ljava/lang/ThreadGroup;Ljava/lang/String;)V");

        OMElysiaNativeValue values[2];
        values[0].l = threadObjects.systemGroup;
        values[1].l = tc->interface.NewStringUTF("main");
        auto obj = tc->interface.NewObjectA(tg, mthd, values);
        threadObjects.mainGroup = obj;
    }

    if (!thisThread.metadata->threadObject)
    {
        auto thrcls = tc->interface.FindClass("java/lang/Thread");
        auto throbj = tc->interface.AllocObject(thrcls);
        tc->interface.SetLongField(throbj, tc->interface.GetFieldID(thrcls, "eetop", "J"),
                                   static_cast<jlong>(reinterpret_cast<uintptr_t>(thisThread.metadata)));
        tc->interface.SetIntField(throbj, tc->interface.GetFieldID(thrcls, "priority", "I"), 5);
        tc->interface.SetObjectField(throbj, tc->interface.GetFieldID(thrcls, "name", "Ljava/lang/String;"),
                                     tc->interface.NewStringUTF(thisThread.metadata->threadName.c_str()));
        tc->interface.SetObjectField(throbj, tc->interface.GetFieldID(thrcls, "group", "Ljava/lang/ThreadGroup;"),
                                     threadObjects.mainGroup);
        tc->interface.SetIntField(throbj, tc->interface.GetFieldID(thrcls, "threadStatus", "I"), 1); // thread runnable

        thisThread.metadata->threadObject = throbj;
    }
}

void OMElysium::throwException(OMElysiaOop *oop)
{
    thisThread.metadata->currentException = oop;
    thisThread.metadata->haveException = true;
}
} // namespace openminecraft::vm::elysia
