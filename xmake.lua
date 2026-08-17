-- Set language standard
set_languages("c++17")

add_rules("mode.release")
add_rules("mode.releasedbg")
add_rules("mode.minsizerel")
add_rules("mode.debug")

set_policy("build.sanitizer.address", false)
set_policy("build.sanitizer.memory", false)
set_policy("build.sanitizer.leak", false)
set_policy("build.sanitizer.undefined", false)

set_policy("build.optimization.lto", false)

-- utils and custom libraries
includes("utils.lua")
includes("extlibs/vulkan.lua")
includes("extlibs/sdl_port.lua")
includes("extlibs/cares.lua")

-- fix libffi compile exception
if is_plat("linux") and is_arch("riscv64", "ppc64", "s390x") then
	includes("extlibs/libffi.lua")
end

if is_plat("harmony") then
	includes("extlibs/libffi_port.lua")
end

--------------------------------------------------------------------------------
-- Platform configs
--------------------------------------------------------------------------------

if not is_plat("windows", "mingw") then
	add_ldflags("-rdynamic")
end

-- Platform macros
if get_config("arch") ~= nil then
	add_defines('OM_ARCH="' .. get_config("arch") .. '"')
end
if not is_plat("windows", "mingw") then
	add_defines("OM_PLATFORM_UNIX=")
end
if is_plat("windows", "mingw") then
	add_defines("OM_PLATFORM_WINDOWS=")
end
if is_plat("mingw") then
	add_defines("OM_PLATFORM_MINGW=")
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
	add_defines("OM_PLATFORM_BSDLIKE=")
end

if apple() then
	add_defines("OM_PLATFORM_APPLE=")
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
	"tinyobjloader",
	"opengl-headers",
	"zlib",
	"bullet3",
	"libffi",
	"c-ares",
	"libsdl3",
	"yoga",
	{ system = false }
)
add_requires("boost", { system = false, configs = { stacktrace = true, asio = true } })
add_requires("fmt", { system = false, configs = { header_only = true } })
add_requires("harfbuzz", { system = false, configs = { freetype = false } })

------------------------------------------------------------------------------
-- Submodules
--------------------------------------------------------------------------------

includes("src/openminecraft-core/xmake.lua")
includes("src/boot/xmake.lua")
includes("tests/xmake.lua")
includes("tools/xmake.lua")

--------------------------------------------------------------------------------
-- openminecraft (core target)
--------------------------------------------------------------------------------

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

add_includedirs("include")
add_files("src/boot/launcher.cpp")
add_rules("utils.bin2obj", { extensions = { ".bundle" } })
add_files("boot.bundle", { zeroend = false })

add_deps("openminecraft-core", "openminecraft-boot")
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
