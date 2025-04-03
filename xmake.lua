set_languages("c++17")

includes("extlibs/libpatches.lua")
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

add_requires("freetype", "harfbuzz", "stb", "yoga", "vulkan-headers", "glm", "bullet3", "vulkan-hpp", { system = false })
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
add_packages("freetype", "harfbuzz", "stb", "yoga", "vulkan-headers", "glm", "bullet3", "vulkan-hpp")
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