archplat_config = {}
archplat_config["plat-ios"] = true
archplat_config["plat-harmony"] = true
archplat_config["plat-desktop"] = true
archplat_config["plat-android"] = true

includes("../../utils.lua")

target("openminecraft-log")
set_kind("static")
add_packages("fmt", "boost")
add_files("*.cpp")
addExtFiles(archplat_config)
add_includedirs("../../include")
