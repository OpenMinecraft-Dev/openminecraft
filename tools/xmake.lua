includes("bundlemaker/xmake.lua")
includes("gitvis/xmake.lua")
if is_plat("windows", "linux", "macos", "bsd") then
	includes("elysiavis/xmake.lua")
end
