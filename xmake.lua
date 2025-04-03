set_languages("c++17")

package("zlib")
    set_homepage("http://www.zlib.net")
    set_description("A Massively Spiffy Yet Delicately Unobtrusive Compression Library")
    set_license("zlib")

    add_urls("https://github.com/madler/zlib/archive/refs/tags/$(version).tar.gz",
             "https://github.com/madler/zlib.git")

    add_versions("v1.2.10", "42cd7b2bdaf1c4570e0877e61f2fdc0bce8019492431d054d3d86925e5058dc5")
    add_versions("v1.2.11", "629380c90a77b964d896ed37163f5c3a34f6e6d897311f1df2a7016355c45eff")
    add_versions("v1.2.12", "d8688496ea40fb61787500e863cc63c9afcbc524468cedeb478068924eb54932")
    add_versions("v1.2.13", "1525952a0a567581792613a9723333d7f8cc20b87a81f920fb8bc7e3f2251428")
    add_versions("v1.3", "b5b06d60ce49c8ba700e0ba517fa07de80b5d4628a037f4be8ad16955be7a7c0")
    add_versions("v1.3.1", "17e88863f3600672ab49182f217281b6fc4d3c762bde361935e436a95214d05c")

    add_configs("zutil", {description = "Export zutil.h api", default = false, type = "boolean"})

    if is_plat("mingw") and is_subhost("msys") then
        add_extsources("pacman::zlib")
    elseif is_plat("linux") then
        add_extsources("pacman::zlib", "apt::zlib1g-dev")
    elseif is_plat("macosx") then
        add_extsources("brew::zlib")
    end

    on_fetch(function (package, opt)
        if xmake.version():lt("2.8.7") then return end -- disable system find if the bug is present
        if opt.system then
            if not package:is_plat("windows", "mingw") then
                return package:find_package("system::z", {includes = "zlib.h"})
            end
        end
    end)

    on_install(function (package)
        io.writefile("xmake.lua", [[
            includes("@builtin/check")
            add_rules("mode.debug", "mode.release")
            target("zlib")
                set_kind("$(kind)")
                if not is_plat("windows") then
                    set_basename("z")
                end
                add_files("adler32.c")
                add_files("compress.c")
                add_files("crc32.c")
                add_files("deflate.c")
                add_files("gzclose.c")
                add_files("gzlib.c")
                add_files("gzread.c")
                add_files("gzwrite.c")
                add_files("inflate.c")
                add_files("infback.c")
                add_files("inftrees.c")
                add_files("inffast.c")
                add_files("trees.c")
                add_files("uncompr.c")
                add_files("zutil.c")
                add_headerfiles("zlib.h", "zconf.h")
                check_cincludes("Z_HAVE_UNISTD_H", "unistd.h")
                check_cincludes("HAVE_SYS_TYPES_H", "sys/types.h")
                check_cincludes("HAVE_STDINT_H", "stdint.h")
                check_cincludes("HAVE_STDDEF_H", "stddef.h")
                if is_plat("windows") then
                    add_defines("_CRT_SECURE_NO_DEPRECATE")
                    add_defines("_CRT_NONSTDC_NO_DEPRECATE")
                    if is_kind("shared") then
                        add_files("win32/zlib1.rc")
                        add_defines("ZLIB_DLL")
                    end
                else
                    add_defines("ZEXPORT=__attribute__((visibility(\"default\")))")
                    add_defines("_LARGEFILE64_SOURCE=1")
                end
        ]])
        import("package.tools.xmake").install(package)

        if package:config("shared") and package:is_plat("windows") then
            package:add("defines", "ZLIB_DLL")
        end

        if package:config("zutil") then
            os.cp("zutil.h", package:installdir("include"))
        end
    end)
package_end()

package("bullet3")
    set_homepage("http://bulletphysics.org")
    set_description("Bullet Physics SDK.")
    set_license("zlib")

    set_urls("https://github.com/bulletphysics/bullet3/archive/$(version).zip",
             "https://github.com/bulletphysics/bullet3.git")
    add_versions("3.25", "b9bc8d1443637a9084e2b585ed582abf2da3ddad7d768acccfe4ee17aca56bf7")

    add_configs("double_precision", {description = "Enable double precision floats", default = false, type = "boolean"})
    add_configs("extras",           {description = "Build the extras", default = false, type = "boolean"})

    if is_plat("windows", "mingw") then
        add_configs("shared", {description = "Build shared library.", default = false, type = "boolean", readonly = true})
    end
    
    add_deps("cmake")
    add_links("Bullet2FileLoader", "Bullet3Collision", "Bullet3Common", "Bullet3Dynamics", "Bullet3Geometry", "Bullet3OpenCL_clew", "BulletDynamics", "BulletCollision", "BulletInverseDynamics", "BulletSoftBody", "LinearMath")
    add_includedirs("include", "include/bullet")

    if is_plat("mingw") and is_subhost("msys") then
        add_extsources("pacman::bullet")
    elseif is_plat("linux") then
        add_extsources("pacman::bullet", "apt::libbullet-dev")
    elseif is_plat("macosx") then
        add_extsources("brew::bullet")
    end

    on_install(function (package)
        local configs = {"-DBUILD_CPU_DEMOS=OFF", "-DBUILD_OPENGL3_DEMOS=OFF", "-DBUILD_BULLET2_DEMOS=OFF", "-DBUILD_UNIT_TESTS=OFF", "-DINSTALL_LIBS=ON", "-DCMAKE_DEBUG_POSTFIX=", "-DCMAKE_POLICY_VERSION_MINIMUM=3.5"}
        table.insert(configs, "-DCMAKE_BUILD_TYPE=" .. (package:debug() and "Debug" or "Release"))
        table.insert(configs, "-DBUILD_SHARED_LIBS=" .. (package:config("shared") and "ON" or "OFF"))
        table.insert(configs, "-DUSE_DOUBLE_PRECISION=" .. (package:config("double_precision") and "ON" or "OFF"))
        table.insert(configs, "-DBUILD_EXTRAS=" .. (package:config("extras") and "ON" or "OFF"))
        table.insert(configs, "-DUSE_MSVC_RUNTIME_LIBRARY_DLL=ON") -- setting this to ON prevents Bullet from replacing flags
        if package:is_plat("windows") and not package:config("vs_runtime"):endswith("d") then
            table.insert(configs, "-DUSE_MSVC_RELEASE_RUNTIME_ALWAYS=ON") -- required to remove _DEBUG from cmake flags
        end
        import("package.tools.cmake").install(package, configs)
    end)
package_end()

package("freetype")
    set_homepage("https://www.freetype.org")
    set_description("A freely available software library to render fonts.")
    set_license("BSD") -- FreeType License (FTL) is a BSD-style license

    add_urls("https://downloads.sourceforge.net/project/freetype/freetype2/$(version)/freetype-$(version).tar.gz",
             "https://download.savannah.gnu.org/releases/freetype/freetype-$(version).tar.gz", {alias="archive"})
    add_urls("https://gitlab.freedesktop.org/freetype/freetype.git",
             "https://github.com/freetype/freetype.git", {alias = "git"})

    add_versions("archive:2.13.1", "0b109c59914f25b4411a8de2a506fdd18fa8457eb86eca6c7b15c19110a92fa5")
    add_versions("archive:2.13.0", "a7aca0e532a276ea8d85bd31149f0a74c33d19c8d287116ef8f5f8357b4f1f80")
    add_versions("archive:2.12.1", "efe71fd4b8246f1b0b1b9bfca13cfff1c9ad85930340c27df469733bbb620938")
    add_versions("archive:2.11.1", "f8db94d307e9c54961b39a1cc799a67d46681480696ed72ecf78d4473770f09b")
    add_versions("archive:2.11.0", "a45c6b403413abd5706f3582f04c8339d26397c4304b78fa552f2215df64101f")
    add_versions("archive:2.10.4", "5eab795ebb23ac77001cfb68b7d4d50b5d6c7469247b0b01b2c953269f658dac")
    add_versions("archive:2.9.1",  "ec391504e55498adceb30baceebd147a6e963f636eb617424bcfc47a169898ce")
    add_versions("git:2.13.1", "VER-2-13-1")
    add_versions("git:2.13.0", "VER-2-13-0")
    add_versions("git:2.12.1", "VER-2-12-1")
    add_versions("git:2.12.0", "VER-2-12-0")
    add_versions("git:2.11.1", "VER-2-11-1")
    add_versions("git:2.11.0", "VER-2-11-0")
    add_versions("git:2.10.4", "VER-2-10-4")
    add_versions("git:2.9.1",  "VER-2-9-1")

    add_patches("2.11.0", path.join(os.scriptdir(), "patches", "2.11.0", "writing_system.patch"), "3172cf1e50501fc7455d9bb04ef4d5bb35b9712bb635f217f90ae6b2f7532eef")

    if not is_host("windows") then
        add_extsources("pkgconfig::freetype2")
    end

    if is_plat("mingw") and is_subhost("msys") then
        add_extsources("pacman::freetype")
    elseif is_plat("linux") then
        add_extsources("pacman::freetype2", "apt::libfreetype-dev")
    elseif is_plat("macosx") then
        add_extsources("brew::freetype")
    end

    add_configs("bzip2", {description = "Support bzip2 compressed fonts", default = false, type = "boolean"})
    add_configs("png", {description = "Support PNG compressed OpenType embedded bitmaps", default = false, type = "boolean"})
    add_configs("woff2", {description = "Use Brotli library to support decompressing WOFF2 fonts", default = false, type = "boolean"})
    add_configs("zlib", {description = "Support reading gzip-compressed font files", default = true, type = "boolean"})
    add_configs("harfbuzz", {description = "Support harfbuzz", default = false, type = "boolean"})

    add_deps("cmake")
    if is_plat("windows", "mingw") and is_subhost("windows") then
        add_deps("pkgconf")
    elseif is_plat("wasm") then
        add_configs("shared", {description = "Build shared library.", default = false, type = "boolean", readonly = true})
    end

    add_includedirs("include/freetype2")

    on_load(function (package)
        local function add_dep(conf, pkg)
            if package:config(conf) then
                package:add("deps", pkg or conf)
            end
        end

        add_dep("bzip2")
        add_dep("zlib")
        add_dep("png", "libpng")
        add_dep("woff2", "brotli")
        add_dep("harfbuzz")
    end)

    on_install(function (package)
        local configs = {"-DCMAKE_INSTALL_LIBDIR=lib", "-DCMAKE_POLICY_VERSION_MINIMUM=3.5"}
        table.insert(configs, "-DCMAKE_BUILD_TYPE=" .. (package:debug() and "Debug" or "Release"))
        table.insert(configs, "-DBUILD_SHARED_LIBS=" .. (package:config("shared") and "ON" or "OFF"))
        local function add_dep(opt)
            if package:config(opt.conf) then
                if package:version():ge("2.11.1") then
                    table.insert(configs, "-DFT_REQUIRE_" .. opt.cmakewith .. "=ON")
                else
                    table.insert(configs, "-DFT_WITH_" .. opt.cmakewith .. "=ON")
                end

                local lib = package:dep(opt.pkg or opt.conf)
                if lib and not lib:is_system() then
                    local includeconf = opt.cmakeinclude or (opt.cmakewith .. "_INCLUDE_DIRS")
                    local libconf = opt.cmakelib or (opt.cmakewith .. "_LIBRARIES")
                    local fetchinfo = lib:fetch()
                    if fetchinfo then
                        table.insert(configs, "-D" .. includeconf .. "=" .. table.concat(fetchinfo.includedirs or fetchinfo.sysincludedirs, ";"))
                        table.insert(configs, "-D" .. libconf .. "=" .. table.concat(fetchinfo.libfiles, ";"))
                    end
                end
            else
                if package:version():ge("2.11.1") then
                    table.insert(configs, "-DFT_DISABLE_" .. opt.cmakewith .. "=ON")
                else
                    table.insert(configs, "-DCMAKE_DISABLE_FIND_PACKAGE_" .. (opt.cmakedisable or opt.cmakewith) .. "=ON")
                end
            end
        end
        add_dep({conf = "bzip2", cmakewith = "BZIP2", cmakedisable = "BZip2", cmakeinclude = "BZIP2_INCLUDE_DIR"})
        add_dep({conf = "png", pkg = "libpng", cmakewith = "PNG", cmakeinclude = "PNG_PNG_INCLUDE_DIR", cmakelib = "PNG_LIBRARY"})
        add_dep({conf = "woff2", pkg = "brotli", cmakewith = "BROTLI", cmakedisable = "BrotliDec", cmakeinclude = "BROTLIDEC_INCLUDE_DIRS", cmakelib = "BROTLIDEC_LIBRARIES"})
        add_dep({conf = "zlib", cmakewith = "ZLIB", cmakeinclude = "ZLIB_INCLUDE_DIR", cmakelib = "ZLIB_LIBRARY"})
        add_dep({conf = "harfbuzz", pkg = "harfbuzz", cmakewith = "HARFBUZZ", cmakedisable = "HarfBuzz", cmakeinclude = "HarfBuzz_INCLUDE_DIR", cmakelib = "HarfBuzz_LIBRARY"})

        import("package.tools.cmake").install(package, configs)
    end)
package_end()

includes("extlibs/vulkan.lua")

-- includes("extlibs/openal.lua")
if not is_plat("bsd") then
    includes("extlibs/shaderc.lua")
end

if not is_plat("iphoneos", "harmony", "android") then
    includes("extlibs/glfw.lua")
    add_requires("opengl", { system = false })
    if not is_plat("linux", "cross", "bsd", "macosx", "iphoneos", "visionos") then
        add_requires("vulkan-loader", { system = false })
    end
end
if is_plat("iphoneos", "macosx") then 
    add_requires("moltenvk", { config = { shared = false } })
end

add_requires("freetype", "harfbuzz", "stb", "yoga", "xxhash", "vulkan-headers", "glm", "bullet3", "vulkan-hpp", { system = false })
if not is_plat("harmony") then
    add_requires("libsdl3")
end

target("openminecraft")
if is_plat("harmony") then
    add_syslinks("vulkan")
end
if is_plat("iphoneos", "harmony", "android") then
    set_kind("shared")
    add_rules("utils.symbols.export_all")
else 
    set_kind("binary")
end
add_files("src/entrypoint.cpp")
add_packages("freetype", "harfbuzz", "stb", "yoga", "xxhash", "vulkan-headers", "glm", "bullet3", "vulkan-hpp")
if not is_plat("harmony") then
    add_packages("libsdl3")
end
if not is_plat("iphoneos", "harmony", "android") then
    add_packages("opengl")
    if not is_plat("linux", "cross", "bsd", "macosx", "iphoneos", "visionos") then
        add_packages("vulkan-loader")
    end
end
if not is_plat("bsd") then
    add_deps("shaderc")
end
if is_plat("iphoneos", "macosx") then 
    add_packages("moltenvk")
end
if is_plat("linux", "cross", "bsd", "android") then
    add_defines("OM_VULKAN_DYNAMIC=")
end
if is_plat("iphoneos") then
    add_frameworks("OpenGLES")
end