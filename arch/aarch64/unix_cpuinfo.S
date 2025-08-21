.globl cpuinfo_aarch64

cpuinfo_aarch64:
    mrs x0, midr_el1
    ret