-- Set language standard
set_languages("c++17")

add_rules("mode.release")
add_rules("mode.releasedbg")
add_rules("mode.minsizerel")
add_rules("mode.debug")
add_rules("mode.check")

-- utils and custom libraries
includes("utils.lua")
includes("extlibs/vulkan.lua")

--------------------------------------------------------------------------------
-- Platform configs
--------------------------------------------------------------------------------

if not is_plat("windows") then
	add_ldflags("-rdynamic")
end

-- Platform macros
if not is_plat("windows", "mingw") then
	add_defines("OM_PLATFORM_UNIX=")
end
if is_plat("windows", "mingw") then
	add_defines("OM_PLATFORM_WINDOWS=")
end
if is_plat("linux") then
	add_defines("OM_PLATFORM_LINUX=")
end
if is_plat("bsd") then
	add_defines("OM_PLATFORM_BSD=")
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
if is_plat("harmony") then
	add_defines("OM_PLATFORM_HARMONY=")
end
if not mobile() then
	add_defines("OM_PLATFORM_DESKTOP=")
end

if apple() or is_plat("bsd") then
	add_defines("BOOST_STACKTRACE_GNU_SOURCE_NOT_REQUIRED=")
end

-- Vulkan dynamic loading
if vulkandyn() then
	add_defines("OM_VULKAN_DYNAMIC=")
end

--------------------------------------------------------------------------------
-- Package dependency
--------------------------------------------------------------------------------

-- Static vulkan loader
if not mobile() then
	if not is_plat("linux", "cross", "bsd", "macosx", "iphoneos", "visionos", "mingw") then
		add_requires("vulkan-loader", { system = false })
	end
end

-- MoltenVK for apple platforms
if apple() then
	add_requires("moltenvk", { configs = { shared = false } })
end

add_requires(
	"vulkan-headers",
	"glm",
	"vulkan-hpp",
	"shaderc",
	"libsdl3",
	"tinyobjloader",
	"opengl-headers",
	"bullet3",
	"zlib",
	"libffi",
	{ system = false }
)
add_requires("boost", { system = false, configs = { stacktrace = true, asio = true } })
add_requires("fmt", { system = false, configs = { header_only = true } })
add_requires("harfbuzz", { system = false, configs = { freetype = false } })

--------------------------------------------------------------------------------
-- Submodules
--------------------------------------------------------------------------------

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
includes("src/specs/xmake.lua")
includes("src/fontproc/xmake.lua")
includes("tests/xmake.lua")
includes("tools/xmake.lua")

--------------------------------------------------------------------------------
-- openminecraft (core target)
--------------------------------------------------------------------------------

target("openminecraft")
if is_plat("android", "harmony") then
	set_kind("shared")
	add_rules("utils.symbols.export_all")
else
	set_kind("binary")
end

add_includedirs("include")
add_files("launcher/**.cpp")

add_deps(
	"openminecraft-log",
	"openminecraft-vm",
	"openminecraft-binary",
	"openminecraft-mem",
	"openminecraft-io",
	"openminecraft-vfs",
	"openminecraft-boot",
	"openminecraft-util",
	"openminecraft-i18n",
	"openminecraft-renderer",
	"openminecraft-specs",
	"openminecraft-fontproc"
)

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
