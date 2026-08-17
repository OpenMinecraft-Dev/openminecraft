local config = {
	["arch-x86"] = true,
	["arch-x86_64"] = true,
	["arch-aarch64"] = true,
	["plat-unix"] = true,
	["plat-windows"] = true,
}

includes(path.join(os.projectdir(), "utils.lua"))

target("openminecraft-vm")
set_kind("static")
add_packages("fmt", "boost", "libffi")
add_deps("openminecraft-mem", "openminecraft-log", "openminecraft-specs", "openminecraft-vfs")
add_files("**.cpp|arch/**|plat/**")
addExtFiles(config)
add_includedirs(path.join(os.projectdir(), "include"))
