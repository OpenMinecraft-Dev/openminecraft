set_languages("c++17")

package("bullet3")
    set_homepage("http://bulletphysics.org")
    set_description("Bullet Physics SDK.")
    set_license("zlib")

    set_urls("https://github.com/bulletphysics/bullet3/archive/$(version).zip",
             "https://github.com/bulletphysics/bullet3.git")
    add_versions("2.88", "f361d10961021a186b80821cfc1cfafc8dac48ce35f7d5e8de0943af4b3ddce4")
    add_versions("3.05", "e7ef322d8038e397cd6d79145a856cf5b4d558ce091d49b5239d625a46fef0d7")
    add_versions("3.09", "8443894e47167cf7f7b4433a365b428ebeb83ba64d64f2a741ec4d2da4992c3d")
    add_versions("3.24", "1179bcc5cdaf7f73f92f5e8495eaadd6a7216e78cad22f1027e9ce49b7a0bfbe")
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

    on_test(function (package)
        assert(package:check_cxxsnippets({test = [[
            void test(int argc, char** argv) {
                btDefaultCollisionConfiguration collisionConfiguration;
                btCollisionDispatcher dispatcher(&collisionConfiguration);
                btDbvtBroadphase broadphase;
                btSequentialImpulseConstraintSolver constraintSolver;
                btDiscreteDynamicsWorld dynamicWorld(&dispatcher, &broadphase, &constraintSolver, &collisionConfiguration);
                dynamicWorld.setGravity(btVector3(0, -10, 0));

                broadphase.optimize();
            }
        ]]}, {includes = "bullet/btBulletDynamicsCommon.h"}))
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

    on_test(function (package)
        assert(package:has_cfuncs("FT_Init_FreeType", {includes = {"ft2build.h", "freetype/freetype.h"}}))
    end)
package_end()

package("vulkan-hpp")
    set_kind("library", {headeronly = true})
    set_homepage("https://github.com/KhronosGroup/Vulkan-Hpp/")
    set_description("Open-Source Vulkan C++ API")
    set_license("Apache-2.0")

    add_urls("https://github.com/KhronosGroup/Vulkan-Hpp.git")
    -- when adding a new sdk version, please also update vulkan-headers, vulkan-loader, vulkan-utility-libraries, spirv-headers, spirv-reflect, glslang and volk packages
    add_versions("v1.2.180", "bfa6d4765212505c8241a44b97dc5a9ce3aa2969")
    add_versions("v1.2.189", "58ff1da4c03f5f124eb835f41a9dd8fe3c2e8087")
    add_versions("v1.2.198", "d8c9f4f0eee6972622a1c3aabab5ed558d37c1c0")
    add_versions("v1.3.231", "ef609a2f77dd1756e672712f264e76b64acdba61")
    add_versions("v1.3.236", "4848fc8e6a923757fd451e52b992dfac48e30814")
    add_versions("v1.3.240", "83adc3fa57b5d5a75ddfb2ce2a0f7fb3abe4bb9c")
    add_versions("v1.3.244", "1bd3877dcc7f3fbf5a43e4d2f0fcc4ebadf6af85")
    add_versions("v1.3.254", "9f89f760a661ff5d7e1e5cc93de13eb4026307b5")
    add_versions("v1.3.261", "3d27c1736a8d520f4d577d9d41566ce1b1fc346e")
    add_versions("v1.3.268", "d2134fefe22279595aee73752099022222468a60")
    add_versions("v1.3.272", "e621db07719c0c1c738ad39ef400737a750bb23a")
    add_versions("v1.3.275", "1a24b015830c116632a0723f3ccfd1f06009ce12")
    add_versions("v1.3.276", "d4b36b82236e052a5e6e4cea5fe7967d5b565ebc")
    add_versions("v1.3.277", "c5c1994f79298543af93d7956b654bdefdfbdd26")
    add_versions("v1.3.278", "29723f90a127ff08d9099855378162f04b4ffddd")
    add_versions("v1.3.279", "6fb8def27290f8b87d7835457a9c68190aed9a9a")
    add_versions("v1.3.280", "e35acfe75215116029298aebf681170559a4fe6a")
    add_versions("v1.3.281", "88d508b32f207ba85b37fe22fe3732322d1c248d")
    add_versions("v1.3.282", "4bf2835dd1a530291cd2b340a58dd7e369d5c86c")
    add_versions("v1.3.283", "2fbc146feefa43b8201af4b01eb3570110f9fa32")
    add_versions("v1.3.290", "e3b0737d57e81875361bf1943f083eac902dacb7")

    add_configs("modules", {description = "Build with C++20 modules support.", default = false, type = "boolean"})

    on_load(function (package)
        if not package:config("modules") then
            package:add("deps", "cmake")
            if package:is_plat("linux") then
                package:add("extsources", "pacman::vulkan-headers")
            end
        end
    end)


    on_install("windows", "harmony", "cross", "linux", "macosx", "mingw", "android", "iphoneos", "bsd", "visionos", function (package)
        local arch_prev
        local plat_prev
        if (package:is_plat("mingw") or package:is_cross()) and package.plat_set then
            arch_prev = package:arch()
            plat_prev = package:plat()
            package:plat_set(os.host())
            package:arch_set(os.arch())
        end
        io.replace("CMakeLists.txt", "-Werror", "", {plain = true})
        io.replace("CMakeLists.txt", "/WX", "", {plain = true})
        import("package.tools.cmake").build(package, {buildir = "build"})
        if arch_prev and plat_prev then
            package:plat_set(plat_prev)
            package:arch_set(arch_prev)
        end
        os.mkdir("build")
        if is_host("windows") then
            os.cp(path.join("**", "VulkanHppGenerator.exe"), "build")
        else
            os.cp(path.join("**", "VulkanHppGenerator"), "build")
        end
        os.runv(path.join("build", "VulkanHppGenerator"))
        if not package:config("modules") then
            os.cp("Vulkan-Headers/include", package:installdir())
            os.cp("vulkan/*.hpp", package:installdir(path.join("include", "vulkan")))
        else
            io.writefile("xmake.lua", [[ 
                target("vulkan-hpp")
                    set_kind("static")
                    set_languages("c++20")
                    add_headerfiles("Vulkan-Headers/include/(**.h)")
                    add_headerfiles("Vulkan/(**.h)")
                    add_headerfiles("Vulkan-Headers/include/(**.hpp)")
                    add_headerfiles("Vulkan/(**.hpp)")
                    add_includedirs("Vulkan")
                    add_includedirs("Vulkan-Headers/include")
                    add_files("Vulkan/vulkan.cppm", {public = true})
            ]])
            local configs = {}
            import("package.tools.xmake").install(package, configs)
        end
    end)

    on_test(function (package)
        assert(package:check_cxxsnippets({test = [[
            void test() {
                vk::ApplicationInfo ai;
                ai.pApplicationName = "Test";
                ai.applicationVersion = VK_MAKE_API_VERSION(1,0,0,0);
                ai.pEngineName = "Test";
                ai.engineVersion = VK_MAKE_API_VERSION(1,0,0,0);
                ai.apiVersion = VK_API_VERSION_1_0;
            }
        ]]}, {includes = "vulkan/vulkan.hpp", configs = {languages = "c++14"} }))
    end)
package_end()

package("vulkan-loader")
    set_homepage("https://github.com/KhronosGroup/Vulkan-Loader")
    set_description("This project provides the Khronos official Vulkan ICD desktop loader for Windows, Linux, and MacOS.")
    set_license("Apache-2.0")

    add_urls("https://github.com/KhronosGroup/Vulkan-Loader/archive/$(version).tar.gz", {version = function (version) 
        local prefix = "sdk-"
        if version:gt("1.3.261+1") then
            prefix = "vulkan-sdk-"
        end
         return prefix .. version:gsub("%+", ".")
    end})

    add_versions("1.3.290+0", "0cd31fdb9b576e432a85ad4d555fac4f4e5ede22ca37ff534ab67c71cd172644")
    add_versions("1.3.283+0", "59151a3cdbf8dcfe9c2ce4b5bf33358255a197f48d8d0ee8a1d8642ed9ace80f")
    add_versions("1.3.280+0", "f9317667a180257381dcbc74726083af581189f51e10e0246adaa86df075fe16")
    add_versions("1.3.275+0", "f49a2653cd592439c5b4b987ffa0b2577b7fa72b7d344d7a2a89f7d6cb2b342e")
    add_versions("1.3.268+0", "404fa621f1ab2731bcc68bcbff64d8c6de322faad2d87f9198641bd37255fd39")
    add_versions("1.3.261+1", "f85f0ea57b63750d4ddaf6c8649df781c4777006daa3cd772b01e7b5ed02f3f2")
    add_versions("1.3.250+1", "b982ec5fae9af6364816a7c5fcf4d3e5c29bfdca35f4b12ee1f90e492e41adc2")
    add_versions("1.3.246+1", "5ffb79b83ec539233ee793dd3c50aa241bd9bd67103d45d3f4b657f1620b7553")
    add_versions("1.3.239+0", "fa2078408793b2173f174173a8784de56b6bbfbcb5fb958a07e46ef126c7eada")
    add_versions("1.3.236+0", "157d2230b50bb5be3ef9b9467aa90d1c109d5f188a49b11f741246d7ca583bf3")
    add_versions("1.3.231+1", "5226fbc6a90e4405200c8cfdd5733d5e0c6a64e64dcc614c485ea06e03d66578")
    add_versions("1.2.198+0", "7d5d56296dcd88af84ed0fde969038370cac8600c4ef7e328788b7422d9025bb")
    add_versions("1.2.189+1", "1d9f539154d37cea0ca336341c3b25e73d5a5320f2f9c9c55f8309422fe6ec3c")
    add_versions("1.2.182+0", "7088fb6922a3af41efd0499b8e66e971164da1e583410d29f801f991a31b180c")
    add_versions("1.2.162+0", "f8f5ec2485e7fdba3f58c1cde5a25145ece1c6a686c91ba4016b28c0af3f21dd")
    add_versions("1.2.154+1", "889e45f7175d915dd0d702013b8021192e181d20f2ad4021c94006088f1edfe5")

    add_configs("shared", {description = "Build shared library.", default = true, type = "boolean", readonly = true})

    if is_plat("mingw") and is_subhost("msys") then
        add_extsources("pacman::vulkan-loader")
    elseif is_plat("linux") then
        add_extsources("apt::libvulkan-dev", "pacman::vulkan-icd-loader")
        add_deps("libxrandr", "libxrender", "libxcb", "libxkbcommon")
    elseif is_plat("macosx") then
        add_extsources("brew::vulkan-loader")
    end

    on_load("windows", "linux", "macosx", "cross", function (package)
        local sdkver = package:version():split("%+")[1]
        package:add("deps", "vulkan-headers " .. sdkver)
        if not package.is_built or package:is_built() then
            package:add("deps", "cmake")
        end
        if package:is_plat("macosx") then
            package:add("links", "vulkan")
        end
    end)

    on_fetch("macosx", function (package, opt)
        if opt.system then
            import("lib.detect.find_path")
            local libdir = find_path("libvulkan.dylib", "~/VulkanSDK/*/macOS/lib")
            local includedir = find_path("vulkan/vulkan.h", "~/VulkanSDK/*/macOS/include")
            if libdir and includedir then
                return {linkdirs = libdir, links = "vulkan", includedirs = includedir}
            end
        end
    end)

    on_install("windows", "linux", "macosx", "cross", function (package)
        local configs = {"-DBUILD_TESTS=OFF", "-DBUILD_WSI_WAYLAND_SUPPORT=OFF"}
        local vulkan_headers = package:dep("vulkan-headers")
        table.insert(configs, "-DVULKAN_HEADERS_INSTALL_DIR=" .. vulkan_headers:installdir())
        if package:is_plat("linux") then
            import("package.tools.cmake").install(package, configs, {packagedeps = {"libxrandr", "libxrender", "libxcb", "libxkbcommon"}})
        else
            import("package.tools.cmake").install(package, configs)
        end
    end)

    on_test(function (package)
        assert(package:has_cfuncs("vkGetDeviceProcAddr", {includes = "vulkan/vulkan_core.h"}))
    end)
package_end()

includes("extlibs/openal.lua")
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
    add_requires("moltenvk")
end

add_requires("openal-soft-mod", "freetype", "harfbuzz", "stb", "yoga", "xxhash", "opengl-headers", "vulkan-headers", "glm", "bullet3", "vulkan-hpp", { system = false })
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
add_packages("openal-soft-mod", "freetype", "harfbuzz", "stb", "yoga", "xxhash", "opengl-headers", "vulkan-headers", "glm", "bullet3", "vulkan-hpp")
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