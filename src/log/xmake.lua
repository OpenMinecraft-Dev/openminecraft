local config = {
	["plat-ios"] = true,
	["plat-harmony"] = true,
	["plat-desktop"] = true,
	["plat-android"] = true,
}

includes("../../utils.lua")

target("openminecraft-log")
set_kind("static")
add_packages("fmt", "boost")
add_files("*.cpp")
addExtFiles(config)
add_includedirs("../../include")
