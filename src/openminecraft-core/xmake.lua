includes("log/xmake.lua")
includes("vm/xmake.lua")
includes("mem/xmake.lua")
includes("io/xmake.lua")
includes("vfs/xmake.lua")
includes("util/xmake.lua")
includes("i18n/xmake.lua")
includes("renderer/xmake.lua")
includes("specs/xmake.lua")
includes("fontproc/xmake.lua")
includes("world/xmake.lua")
includes("network/xmake.lua")

target("openminecraft-core")
set_kind("static")
add_deps(
	"openminecraft-log",
	"openminecraft-vm",
	"openminecraft-mem",
	"openminecraft-io",
	"openminecraft-vfs",
	"openminecraft-util",
	"openminecraft-i18n",
	"openminecraft-renderer",
	"openminecraft-specs",
	"openminecraft-fontproc",
	"openminecraft-world",
	"openminecraft-network"
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
	"c-ares",
	"yoga",
	{ system = false }
)

add_files("*.cpp")
if not is_plat("windows", "mingw", "bsd") then
	add_syslinks("resolv")
end