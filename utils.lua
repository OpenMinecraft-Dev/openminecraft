function mobile()
    return is_plat("iphoneos", "harmony", "android", "visionos")
end

function apple()
    return is_plat("iphoneos", "macosx", "visionos")
end

function vulkandyn()
    return is_plat("linux", "bsd", "android")
end

if is_plat("harmony") then
package("openal-soft")

    set_homepage("https://openal-soft.org")
    set_description("OpenAL Soft is a software implementation of the OpenAL 3D audio API.")
    set_license("LGPL-2.0")

    add_urls("https://github.com/kcat/openal-soft/archive/refs/tags/$(version).tar.gz", {version = function (version)
        return version:ge("1.21.0") and version or "openal-soft-" .. version
    end})
    add_urls("https://github.com/kcat/openal-soft.git")

    add_versions("1.24.3", "7e1fecdeb45e7f78722b776c5cf30bd33934b961d7fd2a11e0494e064cc631ce")
    add_versions("1.23.1", "dfddf3a1f61059853c625b7bb03de8433b455f2f79f89548cbcbd5edca3d4a4a")
    add_versions("1.22.2", "3e58f3d4458f5ee850039b1a6b4dac2343b3a5985a6a2e7ae2d143369c5b8135")
    add_versions("1.22.0", "814831a8013d7365dfd1917b27f1fb6e723f3be3fe1c6a7ff4516425d8392f68")
    add_versions("1.21.1", "8ac17e4e3b32c1af3d5508acfffb838640669b4274606b7892aa796ca9d7467f")

    if is_plat("mingw") and is_subhost("msys") then
        add_extsources("pacman::openal")
    elseif is_plat("linux") then
        add_extsources("pacman::openal", "apt::libopenal-dev")
    elseif is_plat("macosx") then
        add_extsources("brew::openal-soft")
    end

    add_deps("cmake")
    if is_plat("linux") then
        add_deps("libsndio")
    end

    if is_plat("windows", "mingw") then
        add_syslinks("ole32", "shell32", "user32", "winmm", "kernel32", "Avrt")
        if is_plat("mingw") then
            add_syslinks("uuid")
        end
    elseif is_plat("linux", "cross") then
        add_syslinks("dl", "pthread")
     elseif is_plat("bsd") then
        add_syslinks("stdthreads", "pthread")
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

    on_install("windows", "linux", "mingw", "macosx", "android", "iphoneos", "cross", "bsd", "harmony" , function (package)
        if (package:is_plat("linux") and linuxos.name() == "fedora") or package:is_plat("bsd") then
            -- https://github.com/kcat/openal-soft/issues/864
            io.replace("CMakeLists.txt", "if(HAVE_GCC_PROTECTED_VISIBILITY)", "if(0)", {plain = true})
        end
        local configs = {"-DALSOFT_EXAMPLES=OFF", "-DALSOFT_UTILS=OFF"}
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

package("libsdl3")
    set_homepage("https://www.libsdl.org/")
    set_description("Simple DirectMedia Layer")
    set_license("zlib")

    if is_plat("mingw") and is_subhost("msys") then
        add_extsources("pacman::SDL3")
    elseif is_plat("linux") then
        add_extsources("pacman::sdl3", "apt::libsdl3-dev")
    elseif is_plat("macosx") then
        add_extsources("brew::sdl3")
    end

    set_sourcedir("/home/coder2/harmonydev/SDL_port")

    add_deps("cmake", "egl-headers", "opengl-headers")

    if is_plat("linux", "bsd", "cross") then
        add_configs("x11", {description = "Enables X11 support", default = true, type = "boolean"})
        add_configs("wayland", {description = "Enables Wayland support", default = nil, type = "boolean"})
    end

    if is_plat("wasm") then
        add_cxflags("-sUSE_SDL=0")
    end

    on_load(function (package)
        if package:is_plat("linux", "android", "cross") then
            -- Enable Wayland by default except when cross-compiling (wayland package doesn't support cross-compilation yet)
            if package:config("wayland") == nil and not package:is_cross() then
                package:config_set("wayland", true)
            end
        end

        if package:is_plat("windows") then
            package:add("deps", "ninja")
            package:set("policy", "package.cmake_generator.ninja", true)
        end
        if package:is_plat("linux", "bsd", "cross") and package:config("x11") then
            package:add("deps", "libxext", {private = true})
        end
        if package:is_plat("linux", "bsd", "cross") and package:config("wayland") then
            package:add("deps", "wayland", {private = true})
        end
        local libsuffix = package:is_debug() and "d" or ""
        if not package:config("shared") then
            if package:is_plat("windows", "mingw") then
                package:add("syslinks", "user32", "gdi32", "winmm", "imm32", "ole32", "oleaut32", "version", "uuid", "advapi32", "setupapi", "shell32")
            elseif package:is_plat("linux", "bsd") then
                package:add("syslinks", "pthread", "dl")
                if package:is_plat("bsd") then
                    package:add("syslinks", "usbhid")
                end
            elseif package:is_plat("android") then
                package:add("syslinks", "dl", "log", "android", "GLESv1_CM", "GLESv2", "OpenSLES")
            elseif package:is_plat("harmony") then
		package:add("syslinks", "ace_napi.z", "hilog_ndk.z", "ace_ndk.z", "rawfile.z", "pixelmap_ndk.z")
            elseif package:is_plat("iphoneos", "macosx") then
                package:add("frameworks", "AudioToolbox", "AVFoundation", "CoreAudio", "CoreHaptics", "CoreMedia", "CoreVideo", "Foundation", "GameController", "Metal", "QuartzCore", "CoreFoundation", "UniformTypeIdentifiers")
		       package:add("syslinks", "iconv")
                if package:is_plat("macosx") then
                    package:add("frameworks", "Cocoa", "Carbon", "ForceFeedback", "IOKit")
                else
                    package:add("frameworks", "CoreBluetooth", "CoreGraphics", "CoreMotion", "OpenGLES", "UIKit")
                end
		   end
        end
    end)

    on_install(function (package)
        local configs = {}
        table.insert(configs, "-DCMAKE_BUILD_TYPE=" .. (package:debug() and "Debug" or "Release"))
        table.insert(configs, "-DBUILD_SHARED_LIBS=" .. (package:config("shared") and "ON" or "OFF"))
        table.insert(configs, "-DSDL_TEST_LIBRARY=OFF")
        table.insert(configs, "-DSDL_EXAMPLES=OFF")

        local cflags
        local packagedeps
        if not package:is_plat("wasm") then
            packagedeps = {"egl-headers", "opengl-headers"}
        end

        if package:is_plat("linux", "bsd", "cross") then
            table.insert(packagedeps, "libxext")
            table.insert(packagedeps, "libx11")
            table.insert(packagedeps, "xorgproto")
            table.insert(packagedeps, "wayland")
        elseif package:is_plat("wasm") then
            -- emscripten enables USE_SDL by default which will conflict with libsdl headers
            cflags = {"-sUSE_SDL=0"}
        end

        local includedirs = {}
        for _, depname in ipairs(packagedeps) do
            local dep = package:dep(depname)
            if dep then
                local depfetch = dep:fetch()
                if depfetch then
                    for _, includedir in ipairs(depfetch.includedirs or depfetch.sysincludedirs) do
                        table.insert(includedirs, includedir)
                    end
                end
            end
        end
        if #includedirs > 0 then
            includedirs = table.unique(includedirs)
            table.insert(configs, "-DCMAKE_INCLUDE_PATH=" .. table.concat(includedirs, ";"))
            cflags = cflags or {}
            for _, includedir in ipairs(includedirs) do
                table.insert(cflags, "-I" .. includedir)
            end
        end

	if is_plat("harmony") then
	    table.insert(configs, "-DOHOS=OHOS")
	    table.insert(configs, "-DCMAKE_SYSTEM_NAME=OHOS");
	end

        import("package.tools.cmake").install(package, configs, {cflags = cflags})
    end)

    on_test(function (package)
        assert(package:check_cxxsnippets({test = [[
            #include <SDL3/SDL.h>
            int main(int argc, char** argv) {
                SDL_Init(0);
                SDL_Quit();
                return 0;
            }
        ]]}));
    end)
package_end()
end

package("glslang")
    set_homepage("https://github.com/KhronosGroup/glslang/")
    set_description("Khronos-reference front end for GLSL/ESSL, partial front end for HLSL, and a SPIR-V generator.")
    set_license("Apache-2.0")

    add_urls("https://github.com/KhronosGroup/glslang.git")

    -- when adding a new sdk version, please ensure vulkan-headers, vulkan-hpp, vulkan-loader, vulkan-tools, vulkan-validationlayers, vulkan-utility-libraries, spirv-headers, spirv-reflect, spirv-tools, glslang and volk packages are updated simultaneously
    add_versions("1.2.154+1", "bacaef3237c515e40d1a24722be48c0a0b30f75f")
    add_versions("1.2.162+0", "c594de23cdd790d64ad5f9c8b059baae0ee2941d")
    add_versions("1.2.189+1", "2fb89a0072ae7316af1c856f22663fde4928128a")
    add_versions("1.3.211+0", "9bb8cfffb0eed010e07132282c41d73064a7a609")
    add_versions("1.3.231+1", "5755de46b07e4374c05fb1081f65f7ae1f8cca81")
    add_versions("1.3.236+0", "77551c429f86c0e077f26552b7c1c0f12a9f235e")
    add_versions("1.3.239+0", "ca8d07d0bc1c6390b83915700439fa7719de6a2a")
    add_versions("1.3.246+1", "14e5a04e70057972eef8a40df422e30a3b70e4b5")
    add_versions("1.3.250+1", "d1517d64cfca91f573af1bf7341dc3a5113349c0")
    add_versions("1.3.261+1", "76b52ebf77833908dc4c0dd6c70a9c357ac720bd")
    add_versions("1.3.268+0", "36d08c0d940cf307a23928299ef52c7970d8cee6")
    add_versions("1.3.275+0", "a91631b260cba3f22858d6c6827511e636c2458a")
    add_versions("1.3.280+0", "ee2f5d09eaf8f4e8d0d598bd2172fce290d4ca60")
    add_versions("1.3.283+0", "e8dd0b6903b34f1879520b444634c75ea2deedf5")
    add_versions("1.3.290+0", "fa9c3deb49e035a8abcabe366f26aac010f6cbfb")
    add_versions("1.4.309+0", "7200bc12a8979d13b22cd52de80ffb7d41939615")

    add_patches("1.3.246+1", "https://github.com/KhronosGroup/glslang/commit/1e4955adbcd9b3f5eaf2129e918ca057baed6520.patch", "47893def550f1684304ef7c49da38f0a8fe35c190a3452d3bf58370b3ee7165d")

    add_configs("binaryonly", {description = "Only use binary program.", default = false, type = "boolean"})
    add_configs("exceptions", {description = "Build with exception support.", default = false, type = "boolean"})
    add_configs("rtti",       {description = "Build with RTTI support.", default = false, type = "boolean"})
    add_configs("default_resource_limits",       {description = "Build with default resource limits.", default = false, type = "boolean"})
    if is_plat("wasm") then
        add_configs("shared", {description = "Build shared library.", default = false, type = "boolean", readonly = true})
    end

    add_deps("cmake", "python 3.x", {kind = "binary"})
    add_deps("spirv-tools")
    if is_plat("linux") then
        add_syslinks("pthread")
    end

    add_defines("ENABLE_HLSL")

    on_load(function (package)
        if package:config("binaryonly") then
            package:set("kind", "binary")
        end
    end)

    on_fetch(function (package, opt)
        if opt.system and package:config("binaryonly") then
            return package:find_tool("glslangValidator")
        end
    end)

    on_install(function (package)
        package:addenv("PATH", "bin")
	io.replace("glslang/CMakeLists.txt", "ANDROID", "ANDROID OR OHOS")
        io.replace("CMakeLists.txt", "ENABLE_OPT OFF", "ENABLE_OPT ON")
        io.replace("StandAlone/CMakeLists.txt", "target_link_libraries(glslangValidator ${LIBRARIES})", [[
            target_link_libraries(glslangValidator ${LIBRARIES} SPIRV-Tools-opt SPIRV-Tools-link SPIRV-Tools-reduce SPIRV-Tools)
        ]], {plain = true})
        io.replace("SPIRV/CMakeLists.txt", "target_link_libraries(SPIRV PRIVATE MachineIndependent SPIRV-Tools-opt)", [[
            target_link_libraries(SPIRV PRIVATE MachineIndependent SPIRV-Tools-opt SPIRV-Tools-link SPIRV-Tools-reduce SPIRV-Tools)
        ]], {plain = true})
        -- glslang will add a debug lib postfix for win32 platform, disable this to fix compilation issues under windows
        io.replace("CMakeLists.txt", 'set(CMAKE_DEBUG_POSTFIX "d")', [[
            message(WARNING "Disabled CMake Debug Postfix for xmake package generation")
        ]], {plain = true})
        if package:is_plat("wasm") then
            -- wasm-ld doesn't support --no-undefined
            io.replace("CMakeLists.txt", [[add_link_options("-Wl,--no-undefined")]], "", {plain = true})
        end
        local configs = {"-DENABLE_CTEST=OFF", "-DBUILD_EXTERNAL=OFF"}
        table.insert(configs, "-DCMAKE_BUILD_TYPE=" .. (package:debug() and "Debug" or "Release"))
        if package:is_plat("windows") then
            table.insert(configs, "-DBUILD_SHARED_LIBS=OFF")
            if package:debug() then
                table.insert(configs, "-DCMAKE_COMPILE_PDB_OUTPUT_DIRECTORY=''")
            end
        else
            table.insert(configs, "-DBUILD_SHARED_LIBS=" .. (package:config("shared") and "ON" or "OFF"))
        end
        table.insert(configs, "-DENABLE_EXCEPTIONS=" .. (package:config("exceptions") and "ON" or "OFF"))
        table.insert(configs, "-DENABLE_RTTI=" .. (package:config("rtti") and "ON" or "OFF"))
        table.insert(configs, "-DALLOW_EXTERNAL_SPIRV_TOOLS=ON")
	if is_plat("harmony") then
	    table.insert(configs, "-DOHOS=ON")
	end
        import("package.tools.cmake").install(package, configs, {packagedeps = {"spirv-tools"}})
        if not package:config("binaryonly") then
            package:add("links", "glslang", "MachineIndependent", "GenericCodeGen", "OGLCompiler", "OSDependent", "HLSL", "SPIRV", "SPVRemapper")
        end
        if package:config("default_resource_limits") then
            package:add("links", "glslang", "glslang-default-resource-limits")
        end

        os.cp("glslang/MachineIndependent/**.h", package:installdir("include", "glslang", "MachineIndependent"))
        os.cp("glslang/Include/**.h", package:installdir("include", "glslang", "Include"))

        -- https://github.com/KhronosGroup/glslang/releases/tag/12.3.0
        local bindir = package:installdir("bin")
        local glslangValidator = path.join(bindir, "glslangValidator" .. (is_host("windows") and ".exe" or ""))
        if not os.isfile(glslangValidator) then
            local glslang = path.join(bindir, "glslang" .. (is_host("windows") and ".exe" or ""))
            os.trycp(glslang, glslangValidator)
        end
    end)

    on_test(function (package)
        if not package:is_cross() then
            os.vrun("glslangValidator --version")
        end

        if not package:config("binaryonly") then
            assert(package:has_cxxfuncs("ShInitialize", {configs = {languages = "c++11"}, includes = "glslang/Public/ShaderLang.h"}))
        end
    end)
package_end()
