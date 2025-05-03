function mobile()
    return is_plat("iphoneos", "harmony", "android")
end

function apple()
    return is_plat("iphoneos", "macosx", "visionos")
end

function vulkandyn()
    return is_plat("linux", "bsd", "android")
end