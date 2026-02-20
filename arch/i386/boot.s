; Multiboot 头部定义
MBALIGN  equ  1 << 0
MEMINFO  equ  1 << 1
FLAGS    equ  MBALIGN | MEMINFO
MAGIC    equ  0x1BADB002
CHECKSUM equ -(MAGIC + FLAGS)

section .multiboot
align 4
    dd MAGIC
    dd FLAGS
    dd CHECKSUM

section .bss
align 16
stack_bottom:
resb 16384 ; 16 KB 栈空间
stack_top:

section .text
global _start:function
_start:
    mov esp, stack_top    ; 设置栈指针
    extern kernel_main
    call kernel_main      ; 跳转到 C 语言
    cli
.hang:  hlt               ; 停机
    jmp .hang