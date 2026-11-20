.globl cpuinfo_x86

// geopelia: function with prototype (uint32_t op, int32_t *eax, int32_t *ebx, int32_t *ecx, int32_t *edx)
// in POSIX ABI (x64): 
// op -> rdi
// *eax -> rsi
// *ebx -> rdx
// *ecx -> rcx
// *edx -> r8
cpuinfo_x86:
    // save context in stack
    push %rbx

    movl %edi, %eax           // store arg op (edi) to eax

    mov %rdx, %r9             // redirect arg *ebx (rdx) to r9
    mov %rcx, %r10            // redirect arg *ecx (rcx) to r10
    cpuid
    movl %eax, (%rsi)         // store register eax to arg *eax (rsi)
    movl %ebx, (%r9)          // store register ebx to arg *ebx (r9)
    movl %ecx, (%r10)         // store register ecx to arg *ecx (r10)
    movl %edx, (%r8)          // store register edx to arg *edx (r8)

    // restore context
    pop %rbx

    // function return
    ret

.globl _cpuinfo_x86
_cpuinfo_x86:
    jmp cpuinfo_x86
