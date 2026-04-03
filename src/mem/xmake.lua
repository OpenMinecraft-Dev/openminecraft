includes("../../utils.lua")

target("openminecraft-mem")
set_kind("static")
add_packages("fmt", "boost")
add_deps("openminecraft-log")
add_files("*.cpp")
addExtFiles()
add_includedirs("../../include")
