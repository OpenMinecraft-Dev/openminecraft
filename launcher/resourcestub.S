.section .rodata
.global _binary_boot_bundle_start
.type _binary_boot_bundle_start, @object
_binary_boot_bundle_start:
    .incbin "boot.bundle"
.global _binary_boot_bundle_end
.type _binary_boot_bundle_end, @object
_binary_boot_bundle_end:
