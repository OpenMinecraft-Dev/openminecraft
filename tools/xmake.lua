includes("bundlemaker/xmake.lua")
includes("gitvis/xmake.lua")
if is_plat("windows", "linux", "macos", "bsd", "mingw") then
	includes("elysiavis/xmake.lua")
end
