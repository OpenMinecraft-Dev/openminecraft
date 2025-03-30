set_languages("c++17")

includes("extlibs/openal.lua")
if not is_plat("bsd") then
    includes("extlibs/shaderc.lua")
end

if not is_plat("iphoneos", "harmony", "android") then
    includes("extlibs/glfw.lua")
    add_requires("glfw-mod", "opengl")
end
if is_plat("iphoneos", "macosx") then 
    add_requires("moltenvk")
end

add_requires("openal-soft-mod", "freetype", "harfbuzz", "stb", "yoga", "xxhash", "opengl-headers", "vulkan-headers", "glm", "bullet3", { system = false })

target("openminecraft")
if is_plat("harmony", "android") then
    add_links("vulkan")
end
if is_plat("iphoneos", "harmony", "android") then
    set_kind("shared")
    add_rules("utils.symbols.export_all")
else 
    set_kind("binary")
end
add_files("src/entrypoint.cpp")
add_packages("openal-soft-mod", "freetype", "harfbuzz", "stb", "yoga", "xxhash", "opengl-headers", "vulkan-headers", "glm", "bullet3")
if not is_plat("iphoneos", "harmony", "android") then
    add_packages("glfw-mod", "opengl")
end
if not is_plat("bsd") then
    add_deps("shaderc")
end
if is_plat("iphoneos", "macosx") then 
    add_packages("moltenvk")
end