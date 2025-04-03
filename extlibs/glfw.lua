package("libxau")
    set_homepage("https://www.x.org/")
    set_description("X.Org: A Sample Authorization Protocol for X")

    set_sourcedir(path.join(os.scriptdir(), "libxau"))

    on_install("macosx", "linux", "bsd", "cross", function (package)
        import("package.tools.xmake").install(package, configs)
    end)
package_end()

package("libxdmcp")
set_sourcedir(path.join(os.scriptdir(), "libxdmcp"))

on_install("macosx", "linux", "bsd", "cross", function (package)
    import("package.tools.xmake").install(package, configs)
end)
package_end()

package("glfw-mod")
    set_sourcedir(path.join(os.scriptdir(), "glfw"))

    add_configs("glfw_include", {description = "Choose submodules enabled in glfw", default = "none", type = "string", values = {"none", "vulkan", "glu", "glext", "es2", "es3", "system"}})
    add_configs("x11", {description = "Build support for X11", default = is_plat("linux"), type = "boolean"})
    add_configs("wayland", {description = "Build support for Wayland", default = false, type = "boolean"})

    add_deps("cmake")
    add_deps("opengl", {optional = true})
    if is_plat("macosx") then
        add_frameworks("Cocoa", "IOKit")
    elseif is_plat("windows") then
        add_syslinks("user32", "shell32", "gdi32")
    elseif is_plat("mingw") then
        add_syslinks("gdi32")
    elseif is_plat("android") then
        add_syslinks("dl", "log", "android")
    elseif is_plat("linux") then
        add_syslinks("dl", "pthread")
    end

    on_load(function (package)
        local glfw_include = package:config("glfw_include")
        if glfw_include ~= "system" then
            package:add("defines", "GLFW_INCLUDE_" .. glfw_include:upper())
        end
        if package:config("x11") then
            package:add("deps", "libx11", "libxrandr", "libxrender", "libxinerama", "libxfixes", "libxcursor", "libxi", "libxext")
        end
        if package:config("wayland") then
            package:add("deps", "wayland")
        end
    end)

    on_install("!wasm and !ios", function (package)
        local configs = {"-DGLFW_BUILD_DOCS=OFF", "-DGLFW_BUILD_TESTS=OFF", "-DGLFW_BUILD_EXAMPLES=OFF"}
        table.insert(configs, "-DCMAKE_BUILD_TYPE=" .. (package:is_debug() and "Debug" or "Release"))
        table.insert(configs, "-DBUILD_SHARED_LIBS=" .. (package:config("shared") and "ON" or "OFF"))
        if package:is_plat("windows") then
            table.insert(configs, "-DUSE_MSVC_RUNTIME_LIBRARY_DLL=" .. (package:has_runtime("MT") and "OFF" or "ON"))
        end
        table.insert(configs, "-DGLFW_BUILD_X11=" .. (package:config("x11") and "ON" or "OFF"))
        table.insert(configs, "-DGLFW_BUILD_WAYLAND=" .. (package:config("wayland") and "ON" or "OFF"))
        if package:is_plat("linux") then
            import("package.tools.cmake").install(package, configs, {packagedeps = {"libxrender", "libxfixes", "libxext", "libx11", "wayland"}})
        else
            import("package.tools.cmake").install(package, configs)
        end
    end)
package_end()

add_requires("glfw-mod", { configs = { shared = false } })
