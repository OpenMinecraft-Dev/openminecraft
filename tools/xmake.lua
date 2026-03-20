includes("bundlemaker/xmake.lua")
if not is_plat("windows", "iphoneos") then
    includes("gitvis/xmake.lua")
end
