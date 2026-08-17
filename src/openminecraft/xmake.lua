target("openminecraft")
if is_plat("android", "harmony") then
	set_kind("shared")
	add_rules("utils.symbols.export_all")
elseif is_plat("iphoneos0") then
	set_kind("binary")
	add_rules("xcode.application")
	add_files("misc/Info.plist")
else
	set_kind("binary")
end

add_includedirs(path.join(os.projectdir(), "include"))
add_files("**.cpp")
add_rules("utils.bin2obj", { extensions = { ".bundle" } })
add_files(path.join(os.projectdir(), "boot.bundle"), { zeroend = false })

add_deps("openminecraft-core")
add_packages(
	"harfbuzz",
	"vulkan-headers",
	"glm",
	"bullet3",
	"vulkan-hpp",
	"shaderc",
	"fmt",
	"boost",
	"nlohmann_json",
	"libsdl3",
	"zlib",
	"c-ares",
	"yoga",
	{ system = false }
)

if not mobile() and not vulkandyn() and not apple() then
	add_packages("vulkan-loader")
end
if apple() then
	add_packages("moltenvk")
	add_frameworks("CoreText", "CoreFoundation", "CoreGraphics")
end

if is_plat("harmony") then
	add_syslinks("vulkan")
	add_links("vulkan")
end

if is_plat("android") then
	add_syslinks("GLESv2")
end

if is_plat("iphoneos") then
	add_frameworks("OpenGLES")
end

if is_plat("macosx") then
	add_frameworks("OpenGL")
end

if is_plat("windows") then
	add_links("opengl32", "dbghelp")
elseif is_plat("mingw") then
	add_links("pthread", "opengl32", "dbghelp")
end