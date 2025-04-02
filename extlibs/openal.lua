package("openal-soft-mod")
    set_sourcedir(path.join(os.scriptdir(), "openal-soft"))

    add_deps("cmake")

    if is_plat("windows", "mingw") then
        add_syslinks("ole32", "shell32", "user32", "winmm", "kernel32", "avrt")
    elseif is_plat("linux", "cross") then
        add_syslinks("dl", "pthread")
     elseif is_plat("bsd", "cross") then
        add_syslinks("pthread")
    elseif is_plat("android", "harmony") then
        add_syslinks("dl", "OpenSLES")
    elseif is_plat("macosx", "iphoneos") then
        add_frameworks("CoreAudio", "CoreFoundation", "AudioToolbox")
    end

    on_load(function (package)
        if not package:config("shared") then
            package:add("defines", "AL_LIBTYPE_STATIC")
        end
    end)

    on_install("windows", "linux", "mingw", "macosx", "android", "iphoneos", "cross", "bsd", "harmony", "visionos", function (package)
        if (package:is_plat("linux") and linuxos.name() == "fedora") or package:is_plat("bsd") then
            -- https://github.com/kcat/openal-soft/issues/864
            io.replace("CMakeLists.txt", "if(HAVE_GCC_PROTECTED_VISIBILITY)", "if(0)", {plain = true})
        end
        local configs = {"-DALSOFT_EXAMPLES=OFF", "-DALSOFT_UTILS=OFF", "-DALSOFT_BACKEND_SNDIO=OFF"}
        if package:config("shared") then
            table.insert(configs, "-DBUILD_SHARED_LIBS=ON")
            table.insert(configs, "-DLIBTYPE=SHARED")
        else
            table.insert(configs, "-DBUILD_SHARED_LIBS=OFF")
            table.insert(configs, "-DLIBTYPE=STATIC")
        end
        table.insert(configs, "-DCMAKE_BUILD_TYPE=" .. (package:debug() and "Debug" or "Release"))
        import("package.tools.cmake").install(package, configs)
    end)

    on_test(function (package)
        assert(package:has_cfuncs("alGetProcAddress", {includes = "AL/al.h"}))
    end)
package_end()

add_requires("openal-soft-mod", { system = false, configs = { shared = false } })