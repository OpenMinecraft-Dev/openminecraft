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
			return is_arch("x86", "i386")
		end,
		["x86_64"] = function()
			return is_arch("x86_64", "x64", "amd64")
		end,
		["aarch64"] = function()
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

	if plat == "ios" then
		return is_plat("iphoneos")
	end

	return is_plat(plat)
end

PLATFORMS = {
	"unix",
	"windows",
	"linux",
	"bsd",
	"macos",
	"android",
	"ios",
	"harmony",
	"desktop",
}
ARCHITECTURES = {
	"x86",
	"x86_64",
	"aarch64",
	"arm",
	"loongarch",
	"mips",
	"mips64",
	"ppc",
	"ppc64",
	"riscv",
	"riscv64",
}

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

	for _, plat in ipairs(PLATFORMS) do
		addExtFilesSub("plat", plat)
	end

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

	for _, arch in ipairs(ARCHITECTURES) do
		addExtFilesSub2("arch", arch, arch == "x86" or arch == "x86_64" or arch == "aarch64")
	end

	for _, plat in ipairs(PLATFORMS) do
		for _, arch in ipairs(ARCHITECTURES) do
			if is_arch_alias(arch) and is_plat_alias(plat) and config["platarch-" .. plat .. "-" .. arch] then
				add_files("platarch/" .. plat .. "-" .. arch .. "/**.cpp")

				print("Added subdirectory platarch-" .. plat .. "-" .. arch)
			end
		end
	end
end
