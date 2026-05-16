archplat_config = {}
archplat_config["arch-x86"] = true
archplat_config["arch-aarch64"] = true
archplat_config["plat-unix"] = true
archplat_config["plat-windows"] = true

includes("../../utils.lua")

target("openminecraft-vm")
set_kind("static")
add_packages("fmt", "boost", "libffi")
add_deps("openminecraft-mem", "openminecraft-log", "openminecraft-binary")
add_files("**.cpp|arch/**|plat/**")
addExtFiles(archplat_config)
add_includedirs("../../include")
