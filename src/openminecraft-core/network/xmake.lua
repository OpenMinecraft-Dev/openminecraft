local config = {
	["plat-unix"] = true,
	["plat-windows"] = true,
}

includes(path.join(os.projectdir(), "utils.lua"))

target("openminecraft-network")
set_kind("static")
add_packages("fmt", "boost")
add_deps("openminecraft-log")
add_files("**.cpp|plat/**")
addExtFiles(config)
add_includedirs(path.join(os.projectdir(), "include"))