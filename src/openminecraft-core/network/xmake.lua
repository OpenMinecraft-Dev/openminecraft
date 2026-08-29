local config = {
	["plat-unix"] = true,
	["plat-windows"] = true,
}

target("openminecraft-network")
set_kind("static")
add_packages("fmt", "boost")
add_deps("openminecraft-log")
add_files("**.cpp")
add_includedirs(path.join(os.projectdir(), "include"))