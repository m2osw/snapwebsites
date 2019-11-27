// add multiple uint64_t with carry
// the returned value is the carry of the last add operation
//
//    // corresponding C function declaration
//    void sub256(uint64_t * dst, uint64_t const * src);
//

    .macro sub_with_borrow offset
        mov         \offset(%rsi), %rax
        sbb         %rax, \offset(%rdi)
    .endm

    .text
    .p2align    4,,15
    .globl      sub256
    .type       sub256, @function
sub256:
    mov         (%rsi), %rax
    sub         %rax, (%rdi)

    sub_with_borrow 8
    sub_with_borrow 16
    sub_with_borrow 24

    ret

// vim: ts=4 sw=4 et
