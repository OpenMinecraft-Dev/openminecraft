includes("../../utils.lua")

target("openminecraft-vm")
set_kind("static")
add_packages("fmt", "boost", "libffi")
add_deps("openminecraft-mem", "openminecraft-log", "openminecraft-io", "openminecraft-binary")
add_files("**.cpp|arch/**|plat/**")
addExtFiles()
add_includedirs("../../include")
