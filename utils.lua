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
-- arch: arch-%(arch_name)
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

	if is_arch("x86", "i386", "x86_64", "x64") then
		if not is_plat("windows", "mingw") then
			add_files("arch/x86/unix_**.S")
		else
			add_files("arch/x86/msvc_**.S")
		end
		add_files("arch/x86/**.cpp")
	elseif is_arch("arm64-v8a", "arm64") then
		if not is_plat("windows", "mingw") then
			add_files("arch/aarch64/unix_**.S")
		else
			add_files("arch/aarch64/msvc_**.S")
		end
		add_files("arch/aarch64/**.cpp")
	elseif is_arch("armeabi", "armv7k", "armeabi-v7a", "arm", "armv7s", "armv7") then
		add_files("arch/arm/unix_**.S")
		add_files("arch/arm/**.cpp")
	elseif is_arch("loong64") then
		add_files("arch/loongarch/unix_**.S")
		add_files("arch/loongarch/**.cpp")
	elseif is_arch("mips64", "mips64el", "mip64") then
		add_files("arch/mips64/unix_**.S")
		add_files("arch/mips64/**.cpp")
	elseif is_arch("mips", "mipsel") then
		add_files("arch/mips/unix_**.S")
		add_files("arch/mips/**.cpp")
	elseif is_arch("ppc") then
		add_files("arch/ppc/unix_**.S")
		add_files("arch/ppc/**.cpp")
	elseif is_arch("ppc64") then
		add_files("arch/ppc64/unix_**.S")
		add_files("arch/ppc64/**.cpp")
	elseif is_arch("riscv") then
		add_files("arch/riscv/unix_**.S")
		add_files("arch/riscv/**.cpp")
	elseif is_arch("riscv64") then
		add_files("arch/riscv64/unix_**.S")
		add_files("arch/riscv64/**.cpp")
	else
		add_files("arch/fallback/unix_**.S")
		add_files("arch/fallback/**.cpp")
	end
end
