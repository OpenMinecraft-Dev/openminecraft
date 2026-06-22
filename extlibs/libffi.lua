package("libffi")
    set_kind("library")
    set_homepage("https://sourceware.org/libffi/")
    set_description("Use libffi from system sysroot")

    on_fetch(function (package, opt)
        local sysroot = get_config("sdk") or "/home/coder2/Downloads/crosstoolchain/sysroot"
        local arch = "riscv64-linux-gnu"  -- 根据你的目标架构调整
        return {
            includedirs = path.join(sysroot, "usr/include"),
            linkdirs = path.join(sysroot, "usr/lib", arch),
            links = "ffi"
        }
    end)
