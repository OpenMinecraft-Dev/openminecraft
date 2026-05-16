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
	addExtFilesSub(is_plat("iphoneos"), "plat", "iphoneos")
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

if is_plat("harmony") then
	package("libsdl3")
	set_homepage("https://www.libsdl.org/")
	set_description("Simple DirectMedia Layer")
	set_license("zlib")

	if is_plat("mingw") and is_subhost("msys") then
		add_extsources("pacman::SDL3")
	elseif is_plat("linux") then
		add_extsources("pacman::sdl3", "apt::libsdl3-dev")
	elseif is_plat("macosx") then
		add_extsources("brew::sdl3")
	end

	set_sourcedir("/home/coder2/harmonydev/SDL_port")

	add_deps("cmake", "egl-headers", "opengl-headers")

	if is_plat("linux", "bsd", "cross") then
		add_configs("x11", { description = "Enables X11 support", default = true, type = "boolean" })
		add_configs("wayland", { description = "Enables Wayland support", default = nil, type = "boolean" })
	end

	if is_plat("wasm") then
		add_cxflags("-sUSE_SDL=0")
	end

	on_load(function(package)
		if package:is_plat("linux", "android", "cross") then
			-- Enable Wayland by default except when cross-compiling (wayland package doesn't support cross-compilation yet)
			if package:config("wayland") == nil and not package:is_cross() then
				package:config_set("wayland", true)
			end
		end

		if package:is_plat("windows") then
			package:add("deps", "ninja")
			package:set("policy", "package.cmake_generator.ninja", true)
		end
		if package:is_plat("linux", "bsd", "cross") and package:config("x11") then
			package:add("deps", "libxext", { private = true })
		end
		if package:is_plat("linux", "bsd", "cross") and package:config("wayland") then
			package:add("deps", "wayland", { private = true })
		end
		local libsuffix = package:is_debug() and "d" or ""
		if not package:config("shared") then
			if package:is_plat("windows", "mingw") then
				package:add(
					"syslinks",
					"user32",
					"gdi32",
					"winmm",
					"imm32",
					"ole32",
					"oleaut32",
					"version",
					"uuid",
					"advapi32",
					"setupapi",
					"shell32"
				)
			elseif package:is_plat("linux", "bsd") then
				package:add("syslinks", "pthread", "dl")
				if package:is_plat("bsd") then
					package:add("syslinks", "usbhid")
				end
			elseif package:is_plat("android") then
				package:add("syslinks", "dl", "log", "android", "GLESv1_CM", "GLESv2", "OpenSLES")
			elseif package:is_plat("harmony") then
				package:add("syslinks", "ace_napi.z", "hilog_ndk.z", "ace_ndk.z", "rawfile.z", "pixelmap_ndk.z")
			elseif package:is_plat("iphoneos", "macosx") then
				package:add(
					"frameworks",
					"AudioToolbox",
					"AVFoundation",
					"CoreAudio",
					"CoreHaptics",
					"CoreMedia",
					"CoreVideo",
					"Foundation",
					"GameController",
					"Metal",
					"QuartzCore",
					"CoreFoundation",
					"UniformTypeIdentifiers"
				)
				package:add("syslinks", "iconv")
				if package:is_plat("macosx") then
					package:add("frameworks", "Cocoa", "Carbon", "ForceFeedback", "IOKit")
				else
					package:add("frameworks", "CoreBluetooth", "CoreGraphics", "CoreMotion", "OpenGLES", "UIKit")
				end
			end
		end
	end)

	on_install(function(package)
		local configs = {}
		table.insert(configs, "-DCMAKE_BUILD_TYPE=" .. (package:debug() and "Debug" or "Release"))
		table.insert(configs, "-DBUILD_SHARED_LIBS=" .. (package:config("shared") and "ON" or "OFF"))
		table.insert(configs, "-DSDL_TEST_LIBRARY=OFF")
		table.insert(configs, "-DSDL_EXAMPLES=OFF")

		local cflags
		local packagedeps
		if not package:is_plat("wasm") then
			packagedeps = { "egl-headers", "opengl-headers" }
		end

		if package:is_plat("linux", "bsd", "cross") then
			table.insert(packagedeps, "libxext")
			table.insert(packagedeps, "libx11")
			table.insert(packagedeps, "xorgproto")
			table.insert(packagedeps, "wayland")
		elseif package:is_plat("wasm") then
			-- emscripten enables USE_SDL by default which will conflict with libsdl headers
			cflags = { "-sUSE_SDL=0" }
		end

		local includedirs = {}
		for _, depname in ipairs(packagedeps) do
			local dep = package:dep(depname)
			if dep then
				local depfetch = dep:fetch()
				if depfetch then
					for _, includedir in ipairs(depfetch.includedirs or depfetch.sysincludedirs) do
						table.insert(includedirs, includedir)
					end
				end
			end
		end
		if #includedirs > 0 then
			includedirs = table.unique(includedirs)
			table.insert(configs, "-DCMAKE_INCLUDE_PATH=" .. table.concat(includedirs, ";"))
			cflags = cflags or {}
			for _, includedir in ipairs(includedirs) do
				table.insert(cflags, "-I" .. includedir)
			end
		end

		if is_plat("harmony") then
			table.insert(configs, "-DOHOS=OHOS")
			table.insert(configs, "-DCMAKE_SYSTEM_NAME=OHOS")
		end

		import("package.tools.cmake").install(package, configs, { cflags = cflags })
	end)

	on_test(function(package)
		assert(package:check_cxxsnippets({
			test = [[
            #include <SDL3/SDL.h>
            int main(int argc, char** argv) {
                SDL_Init(0);
                SDL_Quit();
                return 0;
            }
        ]],
		}))
	end)
	package_end()
end
