#include <stdio.h>
#include <stdlib.h>
#include <sys/mman.h>
#include <pthread.h>
#include <string.h>
int add(int a, int b) { return a + b; }
typedef int (*func_t)(int, int);
int main() {
    const size_t PAGE_SIZE = 4096;
    const size_t CODE_SIZE = PAGE_SIZE; // 申请一页内存作为可执行代码区域
    // 使用 mmap 分配一块可读写、可执行的内存区域
    void *code_ptr = mmap(NULL, PAGE_SIZE, PROT_EXEC | PROT_WRITE, MAP_ANONYMOUS | MAP_PRIVATE | MAP_JIT, -1, 0);
    if (code_ptr == MAP_FAILED) {
        perror("mmap failed");
        exit(EXIT_FAILURE);
    }
    // 向刚分配的内存中写入机器码（这里简单地将返回值设置为参数值加上 42）
#ifdef __aarch64__
    unsigned char code[] = {0x20, 0x00, 0x00, 0x0b, // mov x16, #42
                            0xc0, 0x03, 0x5f, 0xd6}; // ret
#else
    unsigned char code[] = {0x8d, 0x04, 0x37, 0xc3};
#endif
    pthread_jit_write_protect_np(0);
    memcpy(code_ptr, code, sizeof(code));
    mprotect(code_ptr, 8, PROT_EXEC | PROT_READ | PROT_WRITE);
      pthread_jit_write_protect_np(1);
    func_t func = (func_t)code_ptr; // 将函数指针指向 JIT 编译后的代码
    int result = func(100, 42); // 调用 JIT 编译后的函数，并传递参数
    printf("Result: %d\n", result);
    munmap(code_ptr, CODE_SIZE); // 回收分配的内存
    return 0;
}
