// add multiple uint64_t with carry
//
//    // corresponding C++ function declaration
//    namespace snapdatabase {
//    void add512(uint64_t * dst, uint64_t const * src);
//    }
//

    .macro add_with_carry offset
        mov         \offset(%rsi), %rax
        adc         \offset(%rdi), %rax
        mov         %rax, \offset(%rdi)
    .endm

    .text
    .p2align    4,,15
    .globl      _ZN12snapdatabase6add512EPmPKm
    .type       _ZN12snapdatabase6add512EPmPKm, @function
_ZN12snapdatabase6add512EPmPKm:
    mov         (%rsi), %rax
    add         %rax, (%rdi)

    add_with_carry 8
    add_with_carry 16
    add_with_carry 24
    add_with_carry 32
    add_with_carry 40
    add_with_carry 48
    add_with_carry 56

    ret

// vim: ts=4 sw=4 et
