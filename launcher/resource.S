.global _binary_boot_bundle_start
.global _binary_boot_bundle_end
.global __binary_boot_bundle_start
.global __binary_boot_bundle_end
.section __TEXT,__const
_binary_boot_bundle_start:
__binary_boot_bundle_start:
.incbin "boot.bundle"
_binary_boot_bundle_end:
__binary_boot_bundle_end: