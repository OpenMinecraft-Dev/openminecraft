target("openminecraft-world")
set_kind("static")
add_deps("openminecraft-mem", "openminecraft-log")
add_packages(
	"fmt",
	"boost",
	"glm"
)
add_files("**.cpp")
add_includedirs(path.join(os.projectdir(), "include"))
