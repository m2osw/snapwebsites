// add multiple uint64_t with carry
// the returned value is the carry of the last add operation
//
//    // corresponding C function declaration
//    void add512(uint64_t * dst, uint64_t const * src);
//

    .macro sub_with_borrow offset
        mov         \offset(%rsi), %rax
        sbb         %rax, \offset(%rdi)
    .endm

    .text
    .p2align    4,,15
    .globl      sub512
    .type       sub512, @function
sub512:
    mov         (%rsi), %rax
    sub         %rax, (%rdi)

    sub_with_borrow 8
    sub_with_borrow 16
    sub_with_borrow 24
    sub_with_borrow 32
    sub_with_borrow 40
    sub_with_borrow 48
    sub_with_borrow 56

    ret

// vim: ts=4 sw=4 et
