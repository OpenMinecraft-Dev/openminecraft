.globl cpuinfo_x86

// geopelia: function with prototype (uint32_t op, int32_t *eax, int32_t *ebx, int32_t *ecx, int32_t *edx)
// in cdecl (after saving registers) (x86):
// [esp + 0] copy of esi
// [esp + 4] copy of ebx
// [esp + 8] <return address>
// [esp + 12] argument op
// [esp + 16] argument *eax
// [esp + 20] argument *ebx
// [esp + 24] argument *ecx
// [esp + 28] argument *edx
cpuinfo_x86:
    // save context in stack
    push %ebx
    push %esi

    mov +12(%esp), %eax       // store arg op (arg 1) to eax
    cpuid

    mov +16(%esp), %esi
    mov %eax, (%esi)

    mov +20(%esp), %esi
    mov %ebx, (%esi)

    mov +24(%esp), %esi
    mov %ecx, (%esi)

    mov +28(%esp), %esi
    mov %edx, (%esi)

    // restore context
    pop %esi
    pop %ebx

    // function return
    ret

.globl _cpuinfo_x86
_cpuinfo_x86:
    jmp cpuinfo_x86
