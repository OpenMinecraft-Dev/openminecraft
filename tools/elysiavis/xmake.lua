add_requires("ftxui", { system = false })

target("openminecraft-elysiavis")
set_kind("binary")
add_files("**.cpp")
add_deps("openminecraft-log", "openminecraft-vm", "openminecraft-mem")
add_packages("fmt", "ftxui")
add_includedirs("../../include")
