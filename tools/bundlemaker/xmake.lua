target("openminecraft-bundlemaker")
set_kind("binary")
add_files("**.cpp")
if is_plat("windows") then
	add_files("getopt.c")
end
add_deps("openminecraft-specs", "openminecraft-log", "openminecraft-binary", "openminecraft-i18n")
add_packages("fmt")
add_includedirs("../../include")

