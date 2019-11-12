// add multiple uint64_t with carry
// the returned value is the carry of the last add operation
//
//    // corresponding C function declaration
//    int add(uint64_t * a, uint64_t const * b, uint64_t const * c, uint64_t size);
//

    .text
    .p2align    4,,15
    .globl      add
    .type       add, @function
add:
    jrcxz       .done
    clc
    xor         %r8, %r8

    .p2align    4,,10
    .p2align    3
.loop:
    mov         (%rax, %r8, 8), %rdx
    adc         (%rbx, %r8, 8), %rdx
    mov         %rdx, (%rdi, %r8, 8)
    inc         %r8
    loop        .loop

.done:
    setc        %al
    movzx       %al, %rax
    ret

// vim: ts=4 sw=4 et
