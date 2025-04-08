if not is_arch("arm64") then
package("libxau")
    set_homepage("https://www.x.org/")
    set_description("X.Org: A Sample Authorization Protocol for X")

    set_sourcedir(path.join(os.scriptdir(), "libxau"))

    on_install("macosx", "linux", "bsd", "cross", function (package)
        import("package.tools.xmake").install(package)
    end)
package_end()
end