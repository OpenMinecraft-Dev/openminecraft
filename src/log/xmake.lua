includes("../../utils.lua")

target("openminecraft-log")
set_kind("static")
add_packages("fmt", "boost")
add_files("*.cpp")
addExtFiles()
add_includedirs("../../include")
