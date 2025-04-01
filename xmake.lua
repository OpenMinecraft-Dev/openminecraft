set_languages("c++17")

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


    on_install("windows", "harmony", "cross", "linux", "macosx", "mingw", "android", "iphoneos", "bsd", function (package)
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
    add_requires("glfw-mod", "opengl", { system = false })
    if not is_plat("linux", "cross", "bsd", "macosx") then
        add_requires("vulkan-loader", { system = false })
    end
end
if is_plat("iphoneos", "macosx") then 
    add_requires("moltenvk")
end

add_requires("openal-soft-mod", "harfbuzz", "stb", "yoga", "xxhash", "opengl-headers", "vulkan-headers", "glm", "bullet3", "vulkan-hpp", { system = false })
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
add_packages("openal-soft-mod", "harfbuzz", "stb", "yoga", "xxhash", "opengl-headers", "vulkan-headers", "glm", "bullet3", "vulkan-hpp")
if not is_plat("harmony") then
    add_packages("libsdl3")
end
if not is_plat("iphoneos", "harmony", "android") then
    add_packages("glfw-mod", "opengl")
    if not is_plat("linux", "cross", "bsd", "macosx") then
        add_packages("vulkan-loader")
    else
        add_defines("OM_VULKAN_DYNAMIC=")
    end
end
if not is_plat("bsd") then
    add_deps("shaderc")
end
if is_plat("iphoneos", "macosx") then 
    add_packages("moltenvk")
end