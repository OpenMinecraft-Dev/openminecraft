package("c-ares")
set_homepage("https://c-ares.org/")
set_description("A C library for asynchronous DNS requests (including name resolves)")

set_urls("https://github.com/c-ares/c-ares.git")

add_deps("cmake")

on_install(function(package)
	local configs = {}
	table.insert(configs, "-DCARES_STATIC=ON")
	table.insert(configs, "-DCARES_SHARED=OFF")
	table.insert(configs, "-DCMAKE_BUILD_TYPE=" .. (package:debug() and "Debug" or "Release"))
	table.insert(configs, "-DCARES_BUILD_TOOLS=OFF")
	table.insert(configs, "-DCARES_BUILD_TESTS=OFF")
	import("package.tools.cmake").install(package, configs)
end)

on_test(function(package)
	assert(package:has_cfuncs("ares_init", { includes = "ares.h" }))
end)
package_end()
