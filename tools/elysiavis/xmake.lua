target("openminecraft-elysiavis")
set_kind("binary")
add_files("**.cpp")
add_deps("openminecraft-log", "openminecraft-vm", "openminecraft-mem", "openminecraft-specs", "openminecraft-vfs")
add_packages("fmt", "libffi")
if is_mode("debug") then
	add_packages("tracy")
end
add_includedirs("../../include")
