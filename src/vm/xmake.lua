includes("../../utils.lua")

target("openminecraft-vm")
set_kind("static")
add_packages("fmt", "boost")
add_deps("openminecraft-mem", "openminecraft-log")
add_files("**.cpp|arch/**|plat/**")
addExtFiles()
add_includedirs("../../include")
