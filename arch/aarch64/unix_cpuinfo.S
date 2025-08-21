.globl cpuinfo_aarch64

// geopelia: simple function to fetch midr_el1 readonly register
cpuinfo_aarch64:
    mrs x0, midr_el1
    ret