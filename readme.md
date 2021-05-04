# Luma
A PowerPC emitter/dynamic assembler single header library, written in C++

[![Build And Test](https://github.com/wheremyfoodat/Luma/actions/workflows/run_tests.yml/badge.svg)](https://github.com/wheremyfoodat/Luma/actions/workflows/run_tests.yml) [![Best Emitter Test](https://github.com/wheremyfoodat/Luma/actions/workflows/meme.yml/badge.svg)](https://github.com/wheremyfoodat/Luma/actions/workflows/meme.yml)
# Features
- Easy to use interface
- Easy to include - just add the "luma.hpp" file to your projects include directory
- Support for most basic instructions
- Support for most FPU instructions 
- Support for the IBM Gekko/Broadway/Espresso's "Paired Single" SIMD ISA extension
- Support for instructions of... questionable usefulness, including data/instruction cache control instructions, superscalar execution control instructions, etc
- Easily customizable, letting you add custom instructions/pseudo-ops/types and more
- Support for many different helpful pseudo-ops and macros (`liw`, `clrrwi`, `rotlwi`, `rotrwi`, and more)
- Support for common assembler directives (align, db, dh, dw, dd, df32, df64)
- Optionally allows auto-growing of the code buffer (Note: This is slower than using a fixed size buffer, and is also buggy with jumps due to the way they're handled. This will also not work on platforms where execution permissions matter, so you need to handle memory yourself in that case)
- Easy-to-use label system for jumps/branches
- Works on both little and big endian (code is emitted at native endianness)

# TODO
- AltiVec SIMD ISA support
- Rest of the major missing instructions (mostly load/store addressing modes, and paired quantized loads for PS)
- Fix AutoGrow bugs
- Improve automatic memory management

# Hello world example
Assembly
```arm
.text ; this is a text segment
.globl _start

_start:
    lis r3, str@ha ; load top halfword of the string's address into r3
    ori r3, str@l  ; or with low halfword of the address (full address is now in r3)

    mflr r4 ; LR in r4
    addi r1, -4 ; lower stack frame
    stw  r4, (4)r1 ; store LR into stack
    bl printf      ; call printf

    lwu r3, (4)r1 ; load old LR into r3 and restore stack frame again
    mtlr r3        ; restore LR
    blr            ; return

str:
    .db "Hello from PowerPC!", 0
```

Luma
```cpp
#include <iostream>
#include "luma.hpp"
using namespace Luma;
    
typedef void (*JITCallback)(); // A function pointer type for emitter-generated code


int main() {
    const char* str = "Hello from PowerPC!";
    PPCEmitter <FixedSize> gen; // Creates an emitter object with a 64KB code buffer. 
                                //You can also specify the amount of memory you want in the constructor (if any), or do allocation yourself
                                // Replace the "FixedSize" with "AutoGrow" to have your code buffer automatically grow when overflowing (note: slower and buggier)
    auto code = (JITCallback) gen.getCurr(); // Get pointer to emitted code

    // Emit the assembly for hello world
    gen.liw (r3, (uint32_t) str); // liw is an emitter pseudo-op to load a full 32-bit word
    gen.mflr (r4);
    gen.addi (sp, -4);
    gen.stw (r4, sp, 4);
    gen.bl ((void*) printf);

    gen.lwu (r3, sp, 4);
    gen.mtlr (r3);
    gen.blr();

    // Jump to generated code
    (*code)(); // Jump to emitted code
}
```

Branch example
```cpp
    gen.add <true> (r1, r1, r2); // Store r1 + r2 into r1. The "true" means this instructions should affect flags
    const auto label = gen.bne(); // Generate a bne instruction, which returns a label
    gen.li (r3, 0);
    gen.blr();

    setLabel (label); // set the label for the previous bne here
    gen.li (r3, 1);
    gen.blr();
    gen.nop(); // NOP is another emitter pseudo-op
```

This generates the following assembly code
```arm
    add. r1, r1, r2
    bne labl
    li r3, 0
    blr
labl:
    li r3, 1
    blr
```

For backwards branches
```cpp
    const auto ptr = gen.getCurr();
    gen.add (r0, r1, r2);
    const auto label = gen.beq();
    setLabel (label, ptr);
```
This generates
```arm
label:
    add r0, r1, r2
    beq label
```

Documentation soon™
