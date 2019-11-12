// add multiple uint64_t with carry
// the returned value is the carry of the last add operation
//
//    // corresponding C function declaration
//    void add256(uint64_t * dst, uint64_t const * src);
//

    .macro add_with_carry offset
        mov         \offset(%rsi), %rax
        adc         %rax, \offset(%rdi)
    .endm

    .text
    .p2align    4,,15
    .globl      add256
    .type       add256, @function
add256:
    mov         (%rsi), %rax
    add         %rax, (%rdi)

    add_with_carry 8
    add_with_carry 16
    add_with_carry 24

    ret

// vim: ts=4 sw=4 et
