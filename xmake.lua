set_languages("c++17")
add_rules("mode.release")
add_rules("mode.releasedbg")
add_rules("mode.minsizerel")
add_rules("mode.debug")
add_rules("mode.check")

includes("utils.lua")
includes("extlibs/libpatches.lua")
includes("extlibs/vulkan.lua")

if not is_plat("windows") then 
    add_ldflags("-rdynamic")
end

includes("extlibs/shaderc.lua")

if not mobile() then
    if not is_plat("linux", "cross", "bsd", "macosx", "iphoneos", "visionos") then
        add_requires("vulkan-loader", { system = false })
    end
end
if apple() then 
    add_requires("moltenvk", { configs = { shared = false } })
end

add_requires("freetype", "harfbuzz", "stb", "yoga", "vulkan-headers", "glm", "bullet3", "vulkan-hpp", "shaderc", "nlohmann_json", { system = false })
add_requires("boost", { system = false, configs = { stacktrace = true } })
add_requires("fmt", { system = false, configs = { header_only = true } })
if not is_plat("harmonys") then
    add_requires("libsdl2")
end

if apple() then
    add_defines("BOOST_STACKTRACE_GNU_SOURCE_NOT_REQUIRED=")
end
if vulkandyn() then
    add_defines("OM_VULKAN_DYNAMIC=")
end

if not is_plat("windows") then
    add_defines("OM_PLATFORM_UNIX=")
end

if is_plat("windows") then
    add_defines("OM_PLATFORM_WINDOWS=")
end
if is_plat("linux") then
    add_defines("OM_PLATFORM_LINUX=")
end
if is_plat("macosx") then
    add_defines("OM_PLATFORM_MACOS=")
end
if is_plat("android") then
    add_defines("OM_PLATFORM_ANDROID=")
end
if is_plat("iphoneos") then
    add_defines("OM_PLATFORM_IOS=")
end
if not mobile() then
    add_defines("OM_PLATFORM_DESKTOP=")
end

includes("src/log/xmake.lua")
includes("src/vm/xmake.lua")
includes("src/binary/xmake.lua")
includes("src/mem/xmake.lua")
includes("src/io/xmake.lua")
includes("src/boot/xmake.lua")
includes("src/vfs/xmake.lua")
includes("src/util/xmake.lua")
includes("src/i18n/xmake.lua")
includes("src/renderer/xmake.lua")

target("openminecraft-bundlemaker")
set_kind("binary")
add_packages("fmt")
add_files("tools/om_bundle_maker.cpp")

target("openminecraft")
if is_plat("android", "harmonyos") then
    set_kind("shared")
    add_rules("utils.symbols.export_all")
else 
    set_kind("binary")
end

add_includedirs("include")
if is_plat("harmony") then
    add_syslinks("vulkan")
elseif is_plat("android") then
    add_syslinks("GLESv2")
end

add_files("launcher/**.cpp")
add_deps("openminecraft-log", "openminecraft-vm", "openminecraft-binary", "openminecraft-mem", "openminecraft-io", "openminecraft-vfs", "openminecraft-boot", "openminecraft-util", "openminecraft-i18n", "openminecraft-renderer")

add_packages("freetype", "harfbuzz", "stb", "yoga", "vulkan-headers", "glm", "bullet3", "vulkan-hpp", "shaderc", "fmt", "boost", "nlohmann_json", { system = false })
if not is_plat("harmonys") then
    add_packages("libsdl2")
end
if not mobile() and not vulkandyn() and not apple() then
    add_packages("vulkan-loader")
end
if apple() then 
    add_packages("moltenvk")
end
if is_plat("iphoneos") then
    add_frameworks("OpenGLES")
end

if not is_plat("windows") then
    add_defines("OM_PLATFORM_UNIX=")
    add_files("plat/unix/**.cpp")
end

if is_plat("windows") then
    add_files("plat/windows/**.cpp")
end
if is_plat("linux") then
    add_files("plat/linux/**.cpp")
end
if is_plat("macosx") then
    add_files("plat/macos/**.cpp")
end
if is_plat("android") then
    add_files("plat/android/**.cpp")
end
if is_plat("iphoneos") then
    add_files("plat/ios/**.cpp")
end
if not mobile() then
    add_files("plat/desktop/**.cpp")
end

if is_plat("macosx") then
    add_frameworks("OpenGL")
elseif is_plat("windows", "mingw") then
    add_links("opengl32")
elseif is_plat("linux", "cross") then
    add_links("OpenGL")
end
