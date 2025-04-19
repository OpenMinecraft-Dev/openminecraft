set_languages("c++17")
add_rules("mode.release")
add_rules("mode.releasedbg")
add_rules("mode.minsizerel")
add_rules("mode.debug")
add_rules("mode.check")

includes("extlibs/libpatches.lua")
includes("extlibs/vulkan.lua")

if not is_plat("windows") then 
    add_ldflags("-rdynamic")
end

function mobile()
    return is_plat("iphoneos", "harmony", "android")
end

function apple()
    return is_plat("iphoneos", "macosx", "visionos")
end

function vulkandyn()
    return is_plat("linux", "cross", "bsd", "android")
end

includes("extlibs/shaderc.lua")

if not mobile() then
    if not is_plat("linux", "cross", "bsd", "macosx", "iphoneos", "visionos") then
        add_requires("vulkan-loader", { system = false })
    end
end
if apple() then 
    add_requires("moltenvk")
end

add_requires("freetype", "harfbuzz", "stb", "yoga", "vulkan-headers", "glm", "bullet3", "vulkan-hpp", "shaderc", { system = false })
add_requires("boost", { system = false, configs = { stacktrace = true } })
add_requires("fmt", { system = false, configs = { header_only = true } })
if not is_plat("harmonys") then
    add_requires("libsdl3")
end

includes("log/xmake.lua")
includes("vm/xmake.lua")
includes("binary/xmake.lua")

target("openminecraft")
if mobile() then
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

if apple() then
    add_defines("BOOST_STACKTRACE_GNU_SOURCE_NOT_REQUIRED=")
end

add_files("src/**.cpp")
add_deps("openminecraft-log", "openminecraft-vm", "openminecraft-binary")

add_packages("freetype", "harfbuzz", "stb", "yoga", "vulkan-headers", "glm", "bullet3", "vulkan-hpp", "shaderc", "fmt", "boost", { system = false })
if not is_plat("harmonys") then
    add_packages("libsdl3")
end
if not mobile() and not vulkandyn() and not apple() then
    add_packages("vulkan-loader")
end
if apple() then 
    add_packages("moltenvk")
end
if vulkandyn() then
    add_defines("OM_VULKAN_DYNAMIC=")
end
if is_plat("iphoneos") then
    add_frameworks("OpenGLES")
end

if not is_plat("windows") then
    add_defines("OM_PLATFORM_UNIX=")
end

if is_plat("windows") then
    add_defines("OM_PLATFORM_WINDOWS=")
    add_files("plat/windows/**.cpp")
end
if is_plat("linux") then
    add_defines("OM_PLATFORM_LINUX=")
    add_files("plat/linux/**.cpp")
end
if is_plat("macos") then
    add_defines("OM_PLATFORM_MACOS=")
    add_files("plat/macos/**.cpp")
end
if is_plat("android") then
    add_defines("OM_PLATFORM_ANDROID=")
    add_files("plat/android/**.cpp")
end
if is_plat("iphoneos") then
    add_defines("OM_PLATFORM_IOS=")
    add_files("plat/ios/**.cpp")
end
if not mobile() then
    add_defines("OM_PLATFORM_DESKTOP=")
    add_files("plat/desktop/**.cpp")
end

if is_plat("macosx") then
    add_frameworks("OpenGL")
elseif is_plat("windows", "mingw") then
    add_links("opengl32")
elseif is_plat("linux") then
    add_links("OpenGL")
end