function mobile()
	return is_plat("iphoneos", "harmony", "android", "visionos")
end

function apple()
	return is_plat("iphoneos", "macosx", "visionos")
end

function vulkandyn()
	return is_plat("linux", "bsd", "android", "mingw")
end

function is_arch_alias(arch)
	local matchmap = {
		["x86"] = function()
			return is_arch("x86", "i386", "x86_64", "x64", "amd64")
		end,
		["arm64"] = function()
			return is_arch("arm64-v8a", "arm64")
		end,
		["arm"] = function()
			return is_arch("armeabi", "armv7k", "armeabi-v7a", "arm", "armv7s", "armv7")
		end,
		["loongarch"] = function()
			return is_arch("loong64")
		end,
		["mips64"] = function()
			return is_arch("mips64", "mips64el", "mip64")
		end,
		["mips"] = function()
			return is_arch("mips", "mipsel")
		end,
	}

	if matchmap[arch] then
		return matchmap[arch]()
	else
		return is_arch(arch)
	end
end

function is_plat_alias(plat)
	if plat == "unix" then
		return not is_plat("windows", "mingw")
	end

	if plat == "windows" then
		return is_plat("windows", "mingw")
	end

	if plat == "desktop" then
		return not mobile()
	end

	return is_plat(plat)
end

-- platform-dependent & arch-dependent source config
-- platform: plat-$(platform_name)
-- arch: arch-$(arch_name)
function addExtFiles(config)
	if config == nil then
		return
	end

	function addExtFilesSub(type, name)
		if is_plat_alias(name) and config[type .. "-" .. name] then
			add_files(type .. "/" .. name .. "**.cpp")
			print("Added subdirectory " .. type .. "/" .. name)
		end
	end

	addExtFilesSub("plat", "unix")
	addExtFilesSub("plat", "windows")
	addExtFilesSub("plat", "linux")
	addExtFilesSub("plat", "bsd")
	addExtFilesSub("plat", "macos")
	addExtFilesSub("plat", "android")
	addExtFilesSub("plat", "ios")
	addExtFilesSub("plat", "harmony")
	addExtFilesSub("plat", "desktop")

	function addExtFilesSub2(type, name, ismsvc)
		if is_arch_alias(name) and config[type .. "-" .. name] then
			if is_plat("windows", "mingw") and ismsvc then
				print("Added subdirectory assembly (msvc)")
				add_files(type .. "/" .. name .. "/msvc_**.asm")
			else
				print("Added subdirectory assembly (unix)")
				add_files(type .. "/" .. name .. "/unix_**.S")
			end
			add_files(type .. "/" .. name .. "/**.cpp")

			print("Added subdirectory " .. type .. "/" .. name)
		end
	end

	addExtFilesSub2("arch", "x86", true)
	addExtFilesSub2("arch", "aarch64", true)
	addExtFilesSub2("arch", "arm", false)
	addExtFilesSub2("arch", "loongarch", false)
	addExtFilesSub2("arch", "mips64", false)
	addExtFilesSub2("arch", "mips", false)
	addExtFilesSub2("arch", "ppc", false)
	addExtFilesSub2("arch", "ppc64", false)
	addExtFilesSub2("arch", "riscv", false)
	addExtFilesSub2("arch", "riscv64", false)
end
