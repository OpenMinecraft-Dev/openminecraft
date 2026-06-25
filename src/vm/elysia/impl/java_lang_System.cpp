#include "openminecraft/vm/elysia/interface/om_elysia_interface_defs.hpp"
#include "openminecraft/vm/elysia/om_elysia_klass.hpp"
#include "openminecraft/vm/elysia/om_elysia_oopmanager.hpp"
#include "openminecraft/vm/os/om_hardware.hpp"
#include <filesystem>
#include <unordered_map>
#ifdef OM_PLATFORM_WINDOWS
#include <direct.h>
#else
#include <unistd.h>
#endif

namespace openminecraft::vm::elysia::impl
{
extern "C"
{
    OMElysiaNativeHandle *Java_java_lang_System_initProperties(OMElysiaJNIEnv *env, OMElysiaKlass *klass,
                                                               OMElysiaNativeHandle *properties)
    {
        auto kk = env->FindClass("java/util/Hashtable");
        auto kkm = env->GetMethodID(kk, "put", "(Ljava/lang/Object;Ljava/lang/Object;)Ljava/lang/Object;");

        char userdir[1024];
#ifdef OM_PLATFORM_WINDOWS
        (void)_getcwd(userdir, 1024);
#else
        getcwd(userdir, 1024);
#endif

        std::unordered_map<std::string, std::string> propMap = {
            {"file.encoding", "UTF_8"},
            {"file.separator", fmt::format("{}", (char)std::filesystem::path::preferred_separator)},
#ifdef OM_PLATFORM_WINDOWS
            {"path.separator", ";"},
            {"sun.jnu.encoding", "GBK"},
#else
            {"path.separator", ":"},
            {"sun.jnu.encoding", "UTF_8"},
#endif
            {"java.home", std::filesystem::current_path().string()},
            {"line.separator", "\n"},
            {"user.dir", userdir},
            {"os.version", "10.0"}, // TODO: fake versions
        };

        for (auto &pp : propMap)
        {
            OMElysiaNativeValue args[2];
            args[0].l = env->NewStringUTF(pp.first.c_str());
            args[1].l = env->NewStringUTF(pp.second.c_str());
            env->CallObjectMethodA(properties, kkm, args);
        }

        return properties;
    }

    void Java_java_lang_System_arraycopy(OMElysiaJNIEnv *env, OMElysiaKlass *klass, OMElysiaNativeHandle *src,
                                         jint srcPos, OMElysiaNativeHandle *dst, jint dstPos, jint length)
    {
        auto kk = env->GetObjectClass(src)->toArray()->itemLength;
        auto srcraw = env->internal->elysium->oopManager->arrAccess<uint8_t>(handleFetch(src));
        auto dstraw = env->internal->elysium->oopManager->arrAccess<uint8_t>(handleFetch(dst));
        std::memmove(&dstraw[kk * dstPos], &srcraw[kk * srcPos], kk * length);
    }

    void Java_java_lang_System_setIn0(OMElysiaJNIEnv *env, OMElysiaKlass *klass, OMElysiaNativeHandle *in)
    {
        env->SetStaticObjectField(klass, env->GetFieldID(klass, "in", "Ljava/io/InputStream;"), in);
    }

    void Java_java_lang_System_setOut0(OMElysiaJNIEnv *env, OMElysiaKlass *klass, OMElysiaNativeHandle *out)
    {
        env->SetStaticObjectField(klass, env->GetFieldID(klass, "out", "Ljava/io/PrintStream;"), out);
    }

    void Java_java_lang_System_setErr0(OMElysiaJNIEnv *env, OMElysiaKlass *klass, OMElysiaNativeHandle *out)
    {
        env->SetStaticObjectField(klass, env->GetFieldID(klass, "err", "Ljava/io/PrintStream;"), out);
    }

    OMElysiaNativeHandle *Java_java_lang_System_mapLibraryName(OMElysiaJNIEnv *env, OMElysiaKlass *,
                                                               OMElysiaNativeHandle *name)
    {
        return name;
    }

    jlong Java_java_lang_System_nanoTime(OMElysiaJNIEnv *env, OMElysiaKlass *)
    {
        auto now = std::chrono::steady_clock::now().time_since_epoch();
        return std::chrono::duration_cast<std::chrono::nanoseconds>(now).count();
    }

    void Java_java_lang_System_registerNatives(OMElysiaJNIEnv *env, OMElysiaKlass *klass)
    {
        OMElysiaNativeMethod mm[] = {
            {const_cast<char *>("initProperties"), const_cast<char *>("(Ljava/util/Properties;)Ljava/util/Properties;"),
             reinterpret_cast<void *>(Java_java_lang_System_initProperties)},
            {const_cast<char *>("arraycopy"), const_cast<char *>("(Ljava/lang/Object;ILjava/lang/Object;II)V"),
             reinterpret_cast<void *>(Java_java_lang_System_arraycopy)},
            {const_cast<char *>("setIn0"), const_cast<char *>("(Ljava/io/InputStream;)V"),
             reinterpret_cast<void *>(Java_java_lang_System_setIn0)},
            {const_cast<char *>("setOut0"), const_cast<char *>("(Ljava/io/PrintStream;)V"),
             reinterpret_cast<void *>(Java_java_lang_System_setOut0)},
            {const_cast<char *>("setErr0"), const_cast<char *>("(Ljava/io/PrintStream;)V"),
             reinterpret_cast<void *>(Java_java_lang_System_setErr0)},
            {const_cast<char *>("mapLibraryName"), const_cast<char *>("(Ljava/lang/String;)Ljava/lang/String;"),
             reinterpret_cast<void *>(Java_java_lang_System_mapLibraryName)},
            {const_cast<char *>("nanoTime"), const_cast<char *>("()J"),
             reinterpret_cast<void *>(Java_java_lang_System_nanoTime)}};
        env->RegisterNatives(klass, mm, 7);
    }
}
} // namespace openminecraft::vm::elysia::impl
