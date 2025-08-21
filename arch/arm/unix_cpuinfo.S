.globl cpuinfo_arm

// geopelia: simple function to fetch midr_el1 readonly register
cpuinfo_arm:
    mrs x0, midr_el1
    ret