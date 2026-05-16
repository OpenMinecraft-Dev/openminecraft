function mobile()
	return is_plat("iphoneos", "harmony", "android", "visionos")
end

function apple()
	return is_plat("iphoneos", "macosx", "visionos")
end

function vulkandyn()
	return is_plat("linux", "bsd", "android", "mingw")
end

-- platform-dependent & arch-dependent source config
-- platform: plat-$(platform_name)
-- arch: arch-$(arch_name)
function addExtFiles(config)
	if config == nil then
		return
	end

	function addExtFilesSub(cond, type, name)
		if cond and config[type .. "-" .. name] then
			add_files(type .. "/" .. name .. "**.cpp")
			print("Added subdirectory " .. type .. "/" .. name)
		end
	end

	addExtFilesSub(not is_plat("windows", "mingw"), "plat", "unix")
	addExtFilesSub(is_plat("windows", "mingw"), "plat", "windows")
	addExtFilesSub(is_plat("linux"), "plat", "linux")
	addExtFilesSub(is_plat("bsd"), "plat", "bsd")
	addExtFilesSub(is_plat("macosx"), "plat", "macos")
	addExtFilesSub(is_plat("android"), "plat", "android")
	addExtFilesSub(is_plat("iphoneos"), "plat", "ios")
	addExtFilesSub(is_plat("harmony"), "plat", "harmony")
	addExtFilesSub(not mobile(), "plat", "desktop")

	function addExtFilesSub2(cond, type, name, ismsvc)
		if cond and config[type .. "-" .. name] then
			if is_plat("windows", "mingw") and ismsvc then
				add_files(type .. "/" .. name .. "/msvc_**.asm")
			else
				add_files(type .. "/" .. name .. "/unix_**.S")
			end
			add_files(type .. "/" .. name .. "/**.cpp")

			print("Added subdirectory " .. type .. "/" .. name)
		end
	end

	addExtFilesSub2(is_arch("x86", "i386", "x86_64", "x86"), "arch", "x86", true)
	addExtFilesSub2(is_arch("arm64-v8a", "arm64"), "arch", "aarch64", true)
	addExtFilesSub2(is_arch("armeabi", "armv7k", "armeabi-v7a", "arm", "armv7s", "armv7"), "arch", "arm", false)
	addExtFilesSub2(is_arch("loong64"), "arch", "loongarch", false)
	addExtFilesSub2(is_arch("mips64", "mips64el", "mip64"), "arch", "mips64", false)
	addExtFilesSub2(is_arch("mips", "mipsel"), "arch", "mips", false)
	addExtFilesSub2(is_arch("ppc"), "arch", "ppc", false)
	addExtFilesSub2(is_arch("ppc64"), "arch", "ppc64", false)
	addExtFilesSub2(is_arch("riscv"), "arch", "riscv", false)
	addExtFilesSub2(is_arch("riscv64"), "arch", "riscv64", false)
end
