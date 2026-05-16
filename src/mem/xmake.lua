archplat_config = {}
archplat_config["plat-unix"] = true
archplat_config["plat-windows"] = true

includes("../../utils.lua")

target("openminecraft-mem")
set_kind("static")
add_packages("fmt", "boost")
add_deps("openminecraft-log")
add_files("*.cpp")
addExtFiles(archplat_config)
add_includedirs("../../include")
