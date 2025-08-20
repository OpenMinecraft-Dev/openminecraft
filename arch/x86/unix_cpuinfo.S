.globl cpuinfo_x86
// .type cpuinfo_x86, @function

// geopelia: function with prototype (uint32_t op, int32_t *eax, int32_t *ebx, int32_t *ecx, int32_t *edx)
// in POSIX ABI (x64): 
// op -> rdi
// *eax -> rsi
// *ebx -> rdx
// *ecx -> rcx
// *edx -> r8
//
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
#ifdef __x86_64__
    // save context in stack
    push %rbx
    push %rbp
    push %rsp

    movl %edi, %eax           // store arg op (edi) to eax

    mov %rdx, %r9             // redirect arg *ebx (rdx) to r9
    mov %rcx, %r10            // redirect arg *ecx (rcx) to r10
    cpuid
    movl %eax, (%rsi)         // store register eax to arg *eax (rsi)
    movl %ebx, (%r9)          // store register ebx to arg *ebx (r9)
    movl %ecx, (%r10)         // store register ecx to arg *ecx (r10)
    movl %edx, (%r8)          // store register edx to arg *edx (r8)

    // restore context
    pop %rsp
    pop %rbp
    pop %rbx

    // function return
    ret
#else // 32bit implementation
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
#endif
