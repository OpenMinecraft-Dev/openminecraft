set_languages("c++17")
add_rules("mode.release")
add_rules("mode.releasedbg")
add_rules("mode.minsizerel")
add_rules("mode.debug")
add_rules("mode.check")

includes("utils.lua")
includes("extlibs/vulkan.lua")

if not is_plat("windows") then
	add_ldflags("-rdynamic")
end

includes("extlibs/shaderc.lua")

if not mobile() then
	if not is_plat("linux", "cross", "bsd", "macosx", "iphoneos", "visionos", "mingw") then
		add_requires("vulkan-loader", { system = false })
	end
end
if apple() then
	add_requires("moltenvk", { configs = { shared = false } })
end
if not is_plat("harmony", "mingw") then
	add_requires("openal-soft")
end

add_requires(
	"stb",
	"vulkan-headers",
	"glm",
	"vulkan-hpp",
	"shaderc",
	"libsdl3",
	"tinyobjloader",
	"opengl-headers",
	"bullet3",
	{ system = false }
)
add_requires("boost", { system = false, configs = { stacktrace = true, asio = true } })
add_requires("fmt", { system = false, configs = { header_only = true } })
add_requires("harfbuzz", { system = false, configs = { freetype = false } })

if apple() or is_plat("bsd") then
	add_defines("BOOST_STACKTRACE_GNU_SOURCE_NOT_REQUIRED=")
end
if vulkandyn() then
	add_defines("OM_VULKAN_DYNAMIC=")
end

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

target("openminecraft-plat")
set_kind("static")
add_includedirs("include")
if not is_plat("harmony", "mingw") then
	add_packages("openal-soft", { system = false })
end
-- mingw require this
if is_plat("mingw") then
	add_links("dbghelp")
end
add_packages(
	"harfbuzz",
	"stb",
	"vulkan-headers",
	"glm",
	"bullet3",
	"vulkan-hpp",
	"shaderc",
	"fmt",
	"boost",
	"nlohmann_json",
	"libsdl3",
	{ system = false }
)
if not is_plat("windows", "mingw") then
	add_files("plat/unix/**.cpp")
end
if is_plat("windows", "mingw") then
	add_files("plat/windows/**.cpp")
end
if is_plat("linux") then
	add_files("plat/linux/**.cpp")
end
if is_plat("bsd") then
	add_files("plat/bsd/**.cpp")
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
if is_plat("harmony") then
	add_files("plat/harmony/**.cpp")
end
if not mobile() then
	add_files("plat/desktop/**.cpp")
end

if is_arch("x86", "i386", "x86_64", "x64") then
	if not is_plat("windows", "mingw") then
		add_files("arch/x86/unix_**.S")
	else
		add_files("arch/x86/msvc_**.S")
	end
	add_files("arch/x86/**.cpp")
elseif is_arch("arm64-v8a", "arm64") then
	if not is_plat("windows", "mingw") then
		add_files("arch/aarch64/unix_**.S")
	else
		add_files("arch/aarch64/msvc_**.S")
	end
	add_files("arch/aarch64/**.cpp")
elseif is_arch("armeabi", "armv7k", "armeabi-v7a", "arm", "armv7s", "armv7") then
	add_files("arch/arm/unix_**.S")
	add_files("arch/arm/**.cpp")
elseif is_arch("loong64") then
	add_files("arch/loongarch/unix_**.S")
	add_files("arch/loongarch/**.cpp")
elseif is_arch("mips64", "mips64el", "mip64") then
	add_files("arch/mips64/unix_**.S")
	add_files("arch/mips64/**.cpp")
elseif is_arch("mips", "mipsel") then
	add_files("arch/mips/unix_**.S")
	add_files("arch/mips/**.cpp")
elseif is_arch("ppc") then
	add_files("arch/ppc/unix_**.S")
	add_files("arch/ppc/**.cpp")
elseif is_arch("ppc64") then
	add_files("arch/ppc64/unix_**.S")
	add_files("arch/ppc64/**.cpp")
elseif is_arch("riscv") then
	add_files("arch/riscv/unix_**.S")
	add_files("arch/riscv/**.cpp")
elseif is_arch("riscv64") then
	add_files("arch/riscv64/unix_**.S")
	add_files("arch/riscv64/**.cpp")
else
	add_files("arch/fallback/unix_**.S")
	add_files("arch/fallback/**.cpp")
end

includes("tools/bundlemaker/xmake.lua")

target("openminecraft")
if is_plat("android", "harmony") then
	set_kind("shared")
	add_rules("utils.symbols.export_all")
else
	set_kind("binary")
end

add_includedirs("include")
if is_plat("harmony") then
	add_syslinks("vulkan")
	add_links("vulkan")
elseif is_plat("android") then
	add_syslinks("GLESv2")
end

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
	"openminecraft-plat",
	"openminecraft-specs",
	"openminecraft-fontproc"
)

add_packages(
	"harfbuzz",
	"stb",
	"vulkan-headers",
	"glm",
	"bullet3",
	"vulkan-hpp",
	"shaderc",
	"fmt",
	"boost",
	"nlohmann_json",
	"libsdl3",
	{ system = false }
)
if not mobile() and not vulkandyn() and not apple() then
	add_packages("vulkan-loader")
end
if apple() then
	add_packages("moltenvk")
	add_frameworks("CoreText", "CoreFoundation", "CoreGraphics")
end
if is_plat("iphoneos") then
	add_frameworks("OpenGLES")
end

if is_plat("macosx") then
	add_frameworks("OpenGL")
elseif is_plat("windows") then
	add_links("opengl32", "dbghelp")
elseif is_plat("mingw") then
	add_links("pthread", "opengl32", "dbghelp")
end
