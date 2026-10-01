target("openminecraft-test-network")
set_kind("binary")
add_deps("openminecraft-io", "openminecraft-log", "openminecraft-vfs", "openminecraft-network")
add_packages("fmt", "boost", "c-ares")
add_files("network/test-network.cpp")
add_includedirs(path.join(os.projectdir(), "include"))
if not is_plat("android", "bsd", "windows", "mingw") then
    add_syslinks("resolv")
end

if is_plat("windows", "mingw") then
    add_syslinks("dnsapi")
end