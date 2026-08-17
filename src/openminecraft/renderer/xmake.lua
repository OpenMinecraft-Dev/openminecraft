target("openminecraft-renderer")
set_kind("static")
add_deps("openminecraft-mem", "openminecraft-fontproc")
add_packages(
	"zlib",
	"shaderc",
	"glm",
	"fmt",
	"boost",
	"vulkan-hpp",
	"libsdl3",
	"tinyobjloader",
	"opengl-headers",
	"yoga",
	"harfbuzz"
)
add_files("**.cpp")
add_includedirs(path.join(os.projectdir(), "include"))
