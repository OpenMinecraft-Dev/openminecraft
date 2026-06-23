package("libffi")
    set_kind("library")
    set_homepage("https://sourceware.org/libffi/")
    set_description("Use libffi from system sysroot")

    on_fetch(function (package, opt)
        local sysroot = get_config("sdk") or "/home/coder2/Downloads/crosstoolchain/sysroot"
	local arch = get_config("arch")
	local mapping = {
            riscv64 = "riscv64-linux-gnu",
            s390x = "s390x-linux-gnu",
	    ppc64 = "powerpc64le-linux-gnu"
	}
        return {
            includedirs = path.join(sysroot, "usr/include"),
            linkdirs = path.join(sysroot, "usr/lib", mapping[arch]),
            links = "ffi"
        }
    end)
