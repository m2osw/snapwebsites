// add multiple uint64_t with carry
// the returned value is the carry of the last add operation
//
//    // corresponding C function declaration
//    void add128(uint64_t * dst, uint64_t const * src);
//

    .text
    .p2align    4,,15
    .globl      add128
    .type       add128, @function
add128:
    mov         (%rsi), %rax
    add         %rax, (%rdi)
    mov         8(%rsi), %rax
    adc         %rax, 8(%rdi)
    ret

// vim: ts=4 sw=4 et
