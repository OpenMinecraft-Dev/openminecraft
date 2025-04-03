package("harfbuzz")

    set_homepage("https://harfbuzz.github.io/")
    set_description("HarfBuzz is a text shaping library.")
    set_license("MIT")

    add_urls("https://github.com/harfbuzz/harfbuzz/archive/refs/tags/$(version).tar.gz", {excludes = "README"})
    add_urls("https://github.com/harfbuzz/harfbuzz.git")
    
    add_versions("10.1.0", "c758fdce8587641b00403ee0df2cd5d30cbea7803d43c65fddd76224f7b49b88")
    add_versions("10.0.1", "e7358ea86fe10fb9261931af6f010d4358dac64f7074420ca9bc94aae2bdd542")
    add_versions("9.0.0", "b7e481b109d19aefdba31e9f5888aa0cdfbe7608fed9a43494c060ce1f8a34d2")
    add_versions("8.5.0", "7ad8e4e23ce776efb6a322f653978b3eb763128fd56a90252775edb9fd327956")
    add_versions("8.4.0", "9f1ca089813b05944ad1ce8c7e018213026d35dc9bab480a21eb876838396556")
    add_versions("8.3.0", "6a093165442348d99f3307480ea87ed83bdabaf642cdd9548cff6b329e93bfac")
    add_versions("8.1.1", "b16e6bc0fc7e6a218583f40c7d201771f2e3072f85ef6e9217b36c1dc6b2aa25")
    add_versions("8.1.0", "8d544f1b74797b7b4d88f586e3b9202528b3e8c17968d28b7cdde02041bff5a0")
    add_versions("8.0.1", "d54ca67b6a0bf732b66a343566446d7f93df2bb850133f886c0082fb618a06b2")
    add_versions("8.0.0", "a8e8ec6f0befce0bd5345dd741d2f88534685a798002e343a38b7f9b2e00c884")
    add_versions("7.3.0", "7cefc6cc161e9d5c88210dafc43bc733ca3e383fd3dd4f1e6178f81bd41cfaae")
    add_versions("6.0.0", "6d753948587db3c7c3ba8cc4f8e6bf83f5c448d2591a9f7ec306467f3a4fe4fa")
    add_versions("5.3.1", "77c8c903f4539b050a6d3a5be79705c7ccf7b1cb66d68152a651486e261edbd2")
    add_versions("4.4.1", "1a95b091a40546a211b6f38a65ccd0950fa5be38d95c77b5c4fa245130b418e1")
    add_versions("3.1.1", "5283c7f5f1f06ddb5e2e88319f6946ea37d2eb3a574e0f73f6000de8f9aa34e6")
    add_versions("3.0.0", "55f7e36671b8c5569b6438f80efed2fd663298f785ad2819e115b35b5587ef69")
    add_versions("2.9.0", "bf5d5bad69ee44ff1dd08800c58cb433e9b3bf4dad5d7c6f1dec5d1cf0249d04")
    add_versions("2.8.1", "b3f17394c5bccee456172b2b30ddec0bb87e9c5df38b4559a973d14ccd04509d")

    add_configs("icu", {description = "Enable ICU library unicode functions.", default = false, type = "boolean"})
    add_configs("freetype", {description = "Enable freetype interop helpers.", default = true, type = "boolean"})
    add_configs("glib", {description = "Enable glib unicode functions.", default = false, type = "boolean"})

    if is_plat("android") then
        add_deps("cmake")
    else
        add_deps("meson", "ninja")
        if is_plat("windows") then
            add_deps("pkgconf")
        end
    end
    add_includedirs("include", "include/harfbuzz")
    if is_plat("macosx") then
        add_frameworks("CoreText", "CoreFoundation", "CoreGraphics")
    elseif is_plat("bsd", "android") then
        add_configs("freetype", {description = "Enable freetype interop helpers.", default = false, type = "boolean", readonly = true})
    elseif is_plat("wasm") then
        add_configs("shared", {description = "Build shared library.", default = false, type = "boolean", readonly = true})
    end

    on_load(function (package)
        if package:config("icu") then
            package:add("deps", "icu4c")
        end
        if package:config("freetype") then
            package:add("deps", "freetype")
        end
        if package:config("glib") then
            package:add("deps", "glib", "pcre2")
            if package:is_plat("windows") then
                package:add("deps", "libintl")
            elseif package:is_plat("macosx") then
                package:add("deps", "libintl")
                package:add("deps", "libiconv", {system = true})
            elseif package:is_plat("linux") then
                package:add("deps", "libiconv")
            end
        end
    end)

    on_install("android", function (package)
        local configs = {"-DHB_HAVE_GLIB=OFF", "-DHB_HAVE_GOBJECT=OFF"}
        table.insert(configs, "-DCMAKE_BUILD_TYPE=" .. (package:debug() and "Debug" or "Release"))
        table.insert(configs, "-DBUILD_SHARED_LIBS=" .. (package:config("shared") and "ON" or "OFF"))
        table.insert(configs, "-DHB_HAVE_FREETYPE=" .. (package:config("freetype") and "ON" or "OFF"))
        table.insert(configs, "-DHB_HAVE_ICU=" .. (package:config("icu") and "ON" or "OFF"))
        import("package.tools.cmake").install(package, configs)
    end)

    on_install(function (package)
        local configs = {"-Dtests=disabled", "-Ddocs=disabled", "-Dbenchmark=disabled", "-Dcairo=disabled"}
        if package:is_plat("macosx") then
            table.insert(configs, "-Dcoretext=enabled")
        end
        table.insert(configs, "-Ddefault_library=" .. (package:config("shared") and "shared" or "static"))
        table.insert(configs, "-Dicu=" .. (package:config("icu") and "enabled" or "disabled"))
        table.insert(configs, "-Dfreetype=" .. (package:config("freetype") and "enabled" or "disabled"))
        table.insert(configs, "-Dglib=" .. (package:config("glib") and "enabled" or "disabled"))
        table.insert(configs, "-Dgobject=" .. (package:config("glib") and "enabled" or "disabled"))
        import("package.tools.meson").install(package, configs, {packagedeps = {"libintl", "libiconv", "pcre2"}})
    end)
package_end()

package("stb")
    set_kind("library", {headeronly = true})
    set_homepage("https://github.com/nothings/stb")
    set_description("single-file public domain (or MIT licensed) libraries for C/C++")

    add_urls("https://github.com/nothings/stb.git")
    add_versions("2019.02.07", "756166e853a1d16a14fbc132384b98512cfce117")
    add_versions("2021.07.13", "3a1174060a7dd4eb652d4e6854bc4cd98c159200")
    add_versions("2021.09.10", "af1a5bc352164740c1cc1354942b1c6b72eacb8a")
    add_versions("2023.01.30", "5736b15f7ea0ffb08dd38af21067c314d6a3aae9")
    add_versions("2023.12.15", "f4a71b13373436a2866c5d68f8f80ac6f0bc1ffe")
    add_versions("2024.06.01", "013ac3beddff3dbffafd5177e7972067cd2b5083")
    
    if is_plat("mingw") and is_subhost("msys") then
        add_extsources("pacman::stb")
    elseif is_plat("linux") then
        add_extsources("pacman::stb", "apt::libstb-dev")
    end

    add_includedirs("include", "include/stb")

    on_install(function (package)
        os.cp("*.h", package:installdir("include/stb"))
        os.cp("*.c", package:installdir("include/stb"))
    end)
package_end()

package("yoga")
    set_homepage("https://yogalayout.com/")
    set_description("Yoga is a cross-platform layout engine which implements Flexbox. Follow https://twitter.com/yogalayout for updates.")
    set_license("MIT")

    add_urls("https://github.com/facebook/yoga/archive/refs/tags/$(version).tar.gz",
             "https://github.com/facebook/yoga.git")

    add_versions("v3.2.0", "a963392c6c120a35f097b5b793d2b9b6684b94443ff873b0e521649a69a0b607")
    add_versions("v3.1.0", "06ff9e6df9b2388a0c6ef8db55ba9bc2ae75e716e967cd12cf18785f6379159e")
    add_versions("v3.0.4", "ef3ce5106eed03ab2e40dcfe5b868936a647c5f02b7ffd89ffaa5882dca3ef7f")
    add_versions("v3.0.3", "0ae44f7d30f8130cdf63e91293e11e34803afbfd12482fe4ef786435fc7fa8e7")
    add_versions("v3.0.2", "73a81c51d9ceb5b95cd3abcafeb4c840041801d59f5048dacce91fbaab0cc6f9")
    add_versions("v3.0.0", "da4739061315fd5b6442e0658c2541db24ded359f41525359d5e61edb2f45297")
    add_versions("v2.0.1", "4c80663b557027cdaa6a836cc087d735bb149b8ff27cbe8442fc5e09cec5ed92")

    add_configs("shared", {description = "Build shared binaries", default = false, type = "boolean", readonly = true})

    add_deps("cmake")

    if on_check then
        on_check(function (package)
            assert(package:check_cxxsnippets({test = [[
                #include <bit>
                #include <cstdint>
                void test() {
                    constexpr double f64v = 19880124.0; 
                    constexpr auto u64v = std::bit_cast<std::uint64_t>(f64v);
                }
            ]]}, {configs = {languages = "c++20"}}), "package(yoga) Require at least C++20.")
        end)
    end

    on_install(function (package)
        local configs = {}
        table.insert(configs, "-DCMAKE_BUILD_TYPE=" .. (package:is_debug() and "Debug" or "Release"))
        table.insert(configs, "-DBUILD_SHARED_LIBS=" .. (package:config("shared") and "ON" or "OFF"))
        io.replace("CMakeLists.txt", "add_subdirectory(tests)", "", {plain = true})
        import("package.tools.cmake").install(package, configs)
    end)
package_end()

package("glm")
    set_homepage("https://glm.g-truc.net/")
    set_description("OpenGL Mathematics (GLM)")
    set_license("MIT")

    add_urls("https://github.com/g-truc/glm/archive/refs/tags/$(version).tar.gz", 
             {version = function(version) return version:gsub("%+", ".") end})
    add_urls("https://github.com/g-truc/glm.git")

    add_versions("1.0.1", "9f3174561fd26904b23f0db5e560971cbf9b3cbda0b280f04d5c379d03bf234c")
    add_versions("1.0.0", "e51f6c89ff33b7cfb19daafb215f293d106cd900f8d681b9b1295312ccadbd23")
    add_versions("0.9.9+8", "7d508ab72cb5d43227a3711420f06ff99b0a0cb63ee2f93631b162bfe1fe9592")

    add_configs("header_only", {description = "Use header only version.", default = true, type = "boolean"})
    add_configs("cxx_standard", {description = "Select c++ standard to build.", default = "14", type = "string", values = {"98", "11", "14", "17", "20"}})
    add_configs("modules", {description = "Build with C++20 modules support.", default = false, type = "boolean"})

    on_load(function (package)
        if package:config("modules") then
            package:config_set("header_only", false)
            package:config_set("cxx_standard", "20")
        elseif package:config("header_only") then
            package:set("kind", "library", {headeronly = true})
        else
            package:add("deps", "cmake")
        end
    end)

    on_install(function (package)
        if not package:config("modules") then
            if package:config("header_only") then
                os.cp("glm", package:installdir("include"))
            else
                io.replace("CMakeLists.txt", "NOT GLM_DISABLE_AUTO_DETECTION", "FALSE")
                local configs = {"-DGLM_BUILD_TESTS=OFF"}
                table.insert(configs, "-DCMAKE_BUILD_TYPE=" .. (package:debug() and "Debug" or "Release"))
                table.insert(configs, "-DBUILD_SHARED_LIBS=" .. (package:config("shared") and "ON" or "OFF"))
                table.insert(configs, "-DCMAKE_CXX_STANDARD=" .. package:config("cxx_standard"))
                import("package.tools.cmake").install(package, configs)
            end
        else
            io.writefile("xmake.lua", [[ 
                target("glm")
                    set_kind("$(kind)")
                    set_languages("c++20")
                    add_headerfiles("./(glm/**.hpp)")
                    add_headerfiles("./(glm/**.h)")
                    add_headerfiles("./(glm/**.inl)")
                    add_includedirs(".")
                    add_files("glm/**.cpp")
                    add_files("glm/**.cppm", {public = true})
            ]])
            import("package.tools.xmake").install(package)
        end
    end)
package_end()

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