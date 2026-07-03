if is_plat("harmony") then
	package("libsdl3")
	set_homepage("https://www.libsdl.org/")
	set_description("Simple DirectMedia Layer")
	set_license("zlib")

	add_urls("https://github.com/OpenMinecraft-Dev/SDL.git")

	add_deps("cmake", "egl-headers", "opengl-headers")

	on_load(function(package)
		if not package:config("shared") then
			package:add("syslinks", "ace_napi.z", "hilog_ndk.z", "ace_ndk.z", "rawfile.z", "pixelmap_ndk.z", "ohsensor")
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

		table.insert(configs, "-DOHOS=OHOS")
		table.insert(configs, "-DCMAKE_SYSTEM_NAME=OHOS")

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
