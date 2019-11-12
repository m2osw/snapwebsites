// add multiple uint64_t with carry
// the returned value is the carry of the last add operation
//
//    // corresponding C function declaration
//    void sub128(uint64_t * dst, uint64_t const * src);
//

    .text
    .p2align    4,,15
    .globl      sub128
    .type       sub128, @function
sub128:
    mov         (%rsi), %rax
    sub         %rax, (%rdi)
    mov         8(%rsi), %rax
    sbb         %rax, 8(%rdi)
    ret

// vim: ts=4 sw=4 et
