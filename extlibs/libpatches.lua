package("harfbuzz")

set_homepage("https://harfbuzz.github.io/")
set_description("HarfBuzz is a text shaping library.")
set_license("MIT")

add_urls("https://github.com/harfbuzz/harfbuzz/archive/refs/tags/$(version).tar.gz", { excludes = "README" })
add_urls("https://github.com/harfbuzz/harfbuzz.git")

add_versions("10.1.0", "c758fdce8587641b00403ee0df2cd5d30cbea7803d43c65fddd76224f7b49b88")
add_versions("10.0.1", "e7358ea86fe10fb9261931af6f010d4358dac64f7074420ca9bc94aae2bdd542")
add_versions("9.0.0", "b7e481b109d19aefdba31e9f5888aa0cdfbe7608fed9a43494c060ce1f8a34d2")
add_versions("8.5.0", "7ad8e4e23ce776efb6a322f653978b3eb763128fd56a90252775edb9fd327956")
add_versions("8.4.0", "9f1ca089813b05944ad1ce8c7e018213026d35dc9bab480a21eb876838396556")
add_versions("8.3.0", "6a093165442348d99f3307480ea87ed83bdabaf642cdd9548cff6b329e93bfac")
add_versions("8.1.1", "b16e6bc0fc7e6a218583f40c7d201771f2e3072f85ef6e9217b36c1dc6b2aa25")
add_versions("8.1.0", "8d544f1b74797b7b4d88f586e3b9202528b3e8c17968d28b7cdde02041bff5a0")
add_versions("8.0.1", "d54ca67b6a0bf732b66a343566446d7f93df2bb850133f886c0082fb618a06b2")
add_versions("8.0.0", "a8e8ec6f0befce0bd5345dd741d2f88534685a798002e343a38b7f9b2e00c884")
add_versions("7.3.0", "7cefc6cc161e9d5c88210dafc43bc733ca3e383fd3dd4f1e6178f81bd41cfaae")
add_versions("6.0.0", "6d753948587db3c7c3ba8cc4f8e6bf83f5c448d2591a9f7ec306467f3a4fe4fa")
add_versions("5.3.1", "77c8c903f4539b050a6d3a5be79705c7ccf7b1cb66d68152a651486e261edbd2")
add_versions("4.4.1", "1a95b091a40546a211b6f38a65ccd0950fa5be38d95c77b5c4fa245130b418e1")
add_versions("3.1.1", "5283c7f5f1f06ddb5e2e88319f6946ea37d2eb3a574e0f73f6000de8f9aa34e6")
add_versions("3.0.0", "55f7e36671b8c5569b6438f80efed2fd663298f785ad2819e115b35b5587ef69")
add_versions("2.9.0", "bf5d5bad69ee44ff1dd08800c58cb433e9b3bf4dad5d7c6f1dec5d1cf0249d04")
add_versions("2.8.1", "b3f17394c5bccee456172b2b30ddec0bb87e9c5df38b4559a973d14ccd04509d")

add_configs("icu", { description = "Enable ICU library unicode functions.", default = false, type = "boolean" })
add_configs("freetype", { description = "Enable freetype interop helpers.", default = true, type = "boolean" })
add_configs("glib", { description = "Enable glib unicode functions.", default = false, type = "boolean" })

if is_plat("android") then
	add_deps("cmake")
else
	add_deps("meson", "ninja")
	if is_plat("windows") then
		add_deps("pkgconf")
	end
end
add_includedirs("include", "include/harfbuzz")
if is_plat("macosx", "iphoneos") then
	add_frameworks("CoreText", "CoreFoundation", "CoreGraphics")
elseif is_plat("bsd", "android") then
	add_configs(
		"freetype",
		{ description = "Enable freetype interop helpers.", default = false, type = "boolean", readonly = true }
	)
elseif is_plat("wasm") then
	add_configs("shared", { description = "Build shared library.", default = false, type = "boolean", readonly = true })
end

on_load(function(package)
	if package:config("icu") then
		package:add("deps", "icu4c")
	end
	if package:config("freetype") then
		package:add("deps", "freetype")
	end
	if package:config("glib") then
		package:add("deps", "glib", "pcre2")
		if package:is_plat("windows") then
			package:add("deps", "libintl")
		elseif package:is_plat("macosx") then
			package:add("deps", "libintl")
			package:add("deps", "libiconv", { system = true })
		elseif package:is_plat("linux") then
			package:add("deps", "libiconv")
		elseif package:is_plat("mingw") then
			package:add("deps", "libpthread")
		end
	end
end)

on_install(function(package)
	local configs = { "-DHB_HAVE_GLIB=OFF", "-DHB_HAVE_GOBJECT=OFF" }
	table.insert(configs, "-DCMAKE_BUILD_TYPE=" .. (package:debug() and "Debug" or "Release"))
	table.insert(configs, "-DBUILD_SHARED_LIBS=" .. (package:config("shared") and "ON" or "OFF"))
	table.insert(configs, "-DHB_HAVE_FREETYPE=" .. (package:config("freetype") and "ON" or "OFF"))
	table.insert(configs, "-DHB_HAVE_ICU=" .. (package:config("icu") and "ON" or "OFF"))
	import("package.tools.cmake").install(package, configs)
end)
package_end()

package("glm")
set_homepage("https://glm.g-truc.net/")
set_description("OpenGL Mathematics (GLM)")
set_license("MIT")

add_urls("https://github.com/g-truc/glm/archive/refs/tags/$(version).tar.gz", {
	version = function(version)
		return version:gsub("%+", ".")
	end,
})
add_urls("https://github.com/g-truc/glm.git")

add_versions("1.0.1", "9f3174561fd26904b23f0db5e560971cbf9b3cbda0b280f04d5c379d03bf234c")
add_versions("1.0.0", "e51f6c89ff33b7cfb19daafb215f293d106cd900f8d681b9b1295312ccadbd23")
add_versions("0.9.9+8", "7d508ab72cb5d43227a3711420f06ff99b0a0cb63ee2f93631b162bfe1fe9592")

add_configs("header_only", { description = "Use header only version.", default = true, type = "boolean" })
add_configs("cxx_standard", {
	description = "Select c++ standard to build.",
	default = "14",
	type = "string",
	values = { "98", "11", "14", "17", "20" },
})
add_configs("modules", { description = "Build with C++20 modules support.", default = false, type = "boolean" })

on_load(function(package)
	if package:config("modules") then
		package:config_set("header_only", false)
		package:config_set("cxx_standard", "20")
	elseif package:config("header_only") then
		package:set("kind", "library", { headeronly = true })
	else
		package:add("deps", "cmake")
	end
end)

on_install(function(package)
	if not package:config("modules") then
		if package:config("header_only") then
			os.cp("glm", package:installdir("include"))
		else
			io.replace("CMakeLists.txt", "NOT GLM_DISABLE_AUTO_DETECTION", "FALSE")
			local configs = { "-DGLM_BUILD_TESTS=OFF" }
			table.insert(configs, "-DCMAKE_BUILD_TYPE=" .. (package:debug() and "Debug" or "Release"))
			table.insert(configs, "-DBUILD_SHARED_LIBS=" .. (package:config("shared") and "ON" or "OFF"))
			table.insert(configs, "-DCMAKE_CXX_STANDARD=" .. package:config("cxx_standard"))
			import("package.tools.cmake").install(package, configs)
		end
	else
		io.writefile(
			"xmake.lua",
			[[ 
                target("glm")
                    set_kind("$(kind)")
                    set_languages("c++20")
                    add_headerfiles("./(glm/**.hpp)")
                    add_headerfiles("./(glm/**.h)")
                    add_headerfiles("./(glm/**.inl)")
                    add_includedirs(".")
                    add_files("glm/**.cpp")
                    add_files("glm/**.cppm", {public = true})
            ]]
		)
		import("package.tools.xmake").install(package)
	end
end)
package_end()

package("bullet3")
set_homepage("http://bulletphysics.org")
set_description("Bullet Physics SDK.")
set_license("zlib")

set_urls(
	"https://github.com/bulletphysics/bullet3/archive/$(version).zip",
	"https://github.com/bulletphysics/bullet3.git"
)
add_versions("3.25", "b9bc8d1443637a9084e2b585ed582abf2da3ddad7d768acccfe4ee17aca56bf7")

add_configs("double_precision", { description = "Enable double precision floats", default = false, type = "boolean" })
add_configs("extras", { description = "Build the extras", default = false, type = "boolean" })

if is_plat("windows", "mingw") then
	add_configs("shared", { description = "Build shared library.", default = false, type = "boolean", readonly = true })
end

add_deps("cmake")
add_links(
	"Bullet2FileLoader",
	"Bullet3Collision",
	"Bullet3Common",
	"Bullet3Dynamics",
	"Bullet3Geometry",
	"Bullet3OpenCL_clew",
	"BulletDynamics",
	"BulletCollision",
	"BulletInverseDynamics",
	"BulletSoftBody",
	"LinearMath"
)
add_includedirs("include", "include/bullet")

if is_plat("mingw") and is_subhost("msys") then
	add_extsources("pacman::bullet")
elseif is_plat("linux") then
	add_extsources("pacman::bullet", "apt::libbullet-dev")
elseif is_plat("macosx") then
	add_extsources("brew::bullet")
end

on_install(function(package)
	local configs = {
		"-DBUILD_CPU_DEMOS=OFF",
		"-DBUILD_OPENGL3_DEMOS=OFF",
		"-DBUILD_BULLET2_DEMOS=OFF",
		"-DBUILD_UNIT_TESTS=OFF",
		"-DINSTALL_LIBS=ON",
		"-DCMAKE_DEBUG_POSTFIX=",
		"-DCMAKE_POLICY_VERSION_MINIMUM=3.5",
	}
	table.insert(configs, "-DCMAKE_BUILD_TYPE=" .. (package:debug() and "Debug" or "Release"))
	table.insert(configs, "-DBUILD_SHARED_LIBS=" .. (package:config("shared") and "ON" or "OFF"))
	table.insert(configs, "-DUSE_DOUBLE_PRECISION=" .. (package:config("double_precision") and "ON" or "OFF"))
	table.insert(configs, "-DBUILD_EXTRAS=" .. (package:config("extras") and "ON" or "OFF"))
	table.insert(configs, "-DUSE_MSVC_RUNTIME_LIBRARY_DLL=ON") -- setting this to ON prevents Bullet from replacing flags
	if package:is_plat("windows") and not package:config("vs_runtime"):endswith("d") then
		table.insert(configs, "-DUSE_MSVC_RELEASE_RUNTIME_ALWAYS=ON") -- required to remove _DEBUG from cmake flags
	end
	import("package.tools.cmake").install(package, configs)
end)
package_end()

package("fmt")
set_homepage("https://fmt.dev")
set_description(
	"fmt is an open-source formatting library for C++. It can be used as a safe and fast alternative to (s)printf and iostreams."
)
set_license("MIT")

set_urls(
	"https://github.com/fmtlib/fmt/releases/download/$(version)/fmt-$(version).zip",
	"https://github.com/fmtlib/fmt.git"
)

add_versions("11.1.4", "49b039601196e1a765e81c5c9a05a61ed3d33f23b3961323d7322e4fe213d3e6")
add_versions("11.1.3", "7df2fd3426b18d552840c071c977dc891efe274051d2e7c47e2c83c3918ba6df")
add_versions("11.1.2", "ef54df1d4ba28519e31bf179f6a4fb5851d684c328ca051ce5da1b52bf8b1641")
add_versions("11.1.1", "a25124e41c15c290b214c4dec588385153c91b47198dbacda6babce27edc4b45")
add_versions("11.1.0", "e32d42c6be8df768d744bf0e7d4d69c4ccdce0eda44292ba5265add817413f17")
add_versions("11.0.2", "40fc58bebcf38c759e11a7bd8fdc163507d2423ef5058bba7f26280c5b9c5465")
add_versions("11.0.1", "62ca45531814109b5d6cef0cf2fd17db92c32a30dd23012976e768c685534814")
add_versions("11.0.0", "583ce480ef07fad76ef86e1e2a639fc231c3daa86c4aa6bcba524ce908f30699")
add_versions("10.2.1", "312151a2d13c8327f5c9c586ac6cf7cddc1658e8f53edae0ec56509c8fa516c9")
add_versions("10.2.0", "8a942861a94f8461a280f823041cde8f620a6d8b0e0aacc98c15bb5a9dd92399")
add_versions("10.1.1", "b84e58a310c9b50196cda48d5678d5fa0849bca19e5fdba6b684f0ee93ed9d1b")
add_versions("10.1.0", "d725fa83a8b57a3cedf238828fa6b167f963041e8f9f7327649bddc68ae316f4")
add_versions("10.0.0", "4943cb165f3f587f26da834d3056ee8733c397e024145ca7d2a8a96bb71ac281")
add_versions("9.1.0", "cceb4cb9366e18a5742128cb3524ce5f50e88b476f1e54737a47ffdf4df4c996")
add_versions("9.0.0", "fc96dd2d2fdf2bded630787adba892c23cb9e35c6fd3273c136b0c57d4651ad6")
add_versions("8.1.1", "23778bad8edba12d76e4075da06db591f3b0e3c6c04928ced4a7282ca3400e5d")
add_versions("8.0.1", "a627a56eab9554fc1e5dd9a623d0768583b3a383ff70a4312ba68f94c9d415bf")
add_versions("8.0.0", "36016a75dd6e0a9c1c7df5edb98c93a3e77dabcf122de364116efb9f23c6954a")
add_versions("7.1.3", "5d98c504d0205f912e22449ecdea776b78ce0bb096927334f80781e720084c9f")
add_versions("6.2.0", "a4468d528682143dcef2f16068104e03ef50467b0170b6125c9caf777d27bf10")
add_versions("6.0.0", "b4a16b38fa171f15dbfb958b02da9bbef2c482debadf64ac81ec61b5ac422440")
add_versions("5.3.0", "4c0741e10183f75d7d6f730b8708a99b329b2f942dad5a9da3385ab92bb4a15c")

add_patches("10.1.0", "patches/10.1.0/utf8.patch", "3280569bced9ec08933f0ea37b6a4fef4538944d9046fe197ad63e22d1357cd4")

add_configs("header_only", { description = "Use header only version.", default = false, type = "boolean" })
add_configs("unicode", { description = "Enable Unicode support.", default = true, type = "boolean" })

if is_plat("mingw") and is_subhost("msys") then
	add_extsources("pacman::fmt")
elseif is_plat("linux") then
	add_extsources("pacman::fmt", "apt::libfmt-dev")
elseif is_plat("macosx") then
	add_extsources("brew::fmt")
end

on_load(function(package)
	if package:config("header_only") then
		package:add("defines", "FMT_HEADER_ONLY=1")
		package:set("kind", "library", { headeronly = true })
	else
		package:add("deps", "cmake")
	end
	if package:config("shared") then
		local version = package:version()
		if version and version:ge("10") then
			package:add("defines", "FMT_LIB_EXPORT")
		else
			package:add("defines", "FMT_EXPORT")
		end
	end
	if not package:config("unicode") then
		package:add("defines", "FMT_UNICODE=0")
	end
end)

on_install(function(package)
	if package:has_tool("cxx", "cl") and package:config("unicode") then
		package:add("cxxflags", "/utf-8")
	end
	if package:config("header_only") then
		os.cp("include/fmt", package:installdir("include"))
		return
	end
	io.gsub("CMakeLists.txt", 'MASTER_PROJECT AND CMAKE_GENERATOR MATCHES "Visual Studio"', "0")
	local configs = { "-DFMT_TEST=OFF", "-DFMT_DOC=OFF", "-DFMT_FUZZ=OFF", "-DCMAKE_CXX_VISIBILITY_PRESET=default" }
	table.insert(configs, "-DBUILD_SHARED_LIBS=" .. (package:config("shared") and "ON" or "OFF"))
	table.insert(configs, "-DCMAKE_BUILD_TYPE=" .. (package:is_debug() and "Debug" or "Release"))
	table.insert(configs, "-DFMT_UNICODE=" .. (package:config("unicode") and "ON" or "OFF"))
	import("package.tools.cmake").install(package, configs)
end)
package_end()
