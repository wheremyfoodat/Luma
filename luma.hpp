#pragma once
#include <cstring> // For memcpy
#include <cstdint> // For fixed-sized types
#include <cassert> // for assert
#include <utility> // For std::pair
#include <cstdarg> // For std::va_list
#include <fstream> // For making binary dumps

namespace Luma {

// PPC32 registers
enum GPR {
    r0 = 0, zero = 0, // r0
    r1 = 1, sp = 1,   // r1/stack pointer
    r2 = 2, toc = 2,  // r2/Table of Contents register
    r3 = 3, param1 = 3, // r3/first integer parameter register, return value register
    r4 = 4, param2 = 4, // r4/second integer parameter register
    r5 = 5, param3 = 5, // r5/third integer parameter register
    r6 = 6, param4 = 6, // r6/fourth integer parameter register
    r7 = 7, param5 = 7, // r7/fifth integer parameter register
    r8 = 8, param6 = 8, // r8/sixth integer parameter register
    r9 = 9, param7 = 9, // r9/seventh integer parameter register
    r10 = 10, param8 = 10, // r10/eighth integer parameter register
    r11 = 11, ep = 11, // r11 - used for calls via pointer and as an environment register in certain languages
    r12 = 12, // r12 - used for exception handling
    r13 = 13, // reserved - NOT restored across system calls

    // Non-volatiles
    r14 = 14, r15, r16, r17, r18, r19, r20, r21, r22, r23, r24, r25, r26, r27,
    r28, r29, r30, r31 
};

// Condition register fields
enum CR {
    cr0 = 0, // Has a similar use to flags on real architectures for ALU ops
    cr1, // Used for FPU flags
    cr2, cr3,  cr4,  cr5, // Free real estate (cr2, cr3 and cr4 are non-volatile)
    cr6, // Used in some AltiVec ops 
    cr7  // More real estate
};

enum FPR {
    f0 = 0, // Scratch
    f1 = 1, fparam1 = 1, // First floating point parameter / first 8 bytes of a fp scalar return value
    f2 = 2, fparam2 = 2, // Second floating point parameter / second 8 bytes of a fp scalar return value
    f3 = 3, fparam3 = 3, // Third floating point parameter / third 8 bytes of a fp scalar return value
    f4 = 4, fparam4 = 4, // Fourth floating point parameter / fourth 8 bytes of a fp scalar return value
    f5 = 5, fparam5 = 5, // Fifth floating point parameter
    f6 = 6, fparam6 = 6, // Sixth floating point parameter
    f7 = 7, fparam7 = 7, // Seventh floating point parameter
    f8 = 8, fparam8 = 8, // Eighth floating point parameter
    f9 = 9, fparam9 = 9, // Nineth floating point parameter
    f10 = 10, fparam10 = 10, // Tenth floating point parameter
    f11 = 11, fparam11 = 11, // Eleventh floating point parameter
    f12 = 12, fparam12 = 12, // Twelvth floating point parameter
    f13 = 13, fparam13 = 13, // Thirteenth floating point parameter

    // Volatile bois
    f14 = 14, f15 = 15, f16 = 16, f17 = 17,
    f18 = 18, f19 = 19, f20 = 20, f21 = 21,
    f22 = 22, f23 = 23, f24 = 24, f25 = 25,
    f26 = 26, f27 = 27, f28 = 28, f29 = 29,
    f30 = 30, f31 = 31
};

enum VR { // AltiVec Vector Registers
	v0 = 0, v1, v2, v3, v4, v5, v6, v7, v8,
	v9, v10, v11, v12, v13, v14, v15, v16,
	v17, v18, v19, v20, v21, v22, v23, v24,
	v25, v26, v27, v28, v29, v30, v31
};

// Segment registers
enum SR {
    sr0 = 0, sr1, sr2, sr3, sr4, sr5, sr6, sr7, sr8,
    sr9, sr10, sr11, sr12, sr13, sr14, sr15
};

enum class Cond {
    Lt = 0,
    Gt,
    Eq,
    Os,
    Ge,
    Le,
    Ne,
    Oc,
};
    
enum class BranchType {
    Branch14, Branch24
};

enum GrowingMode {
    FixedSize,
    AutoGrow
};

template <GrowingMode growMode = GrowingMode::FixedSize>
class PPCEmitter {
    uint32_t* code = nullptr; // Pointer to the code buffer
    uint32_t* currentPointer = nullptr; // Pointer to the current address in the code buffer
    uintptr_t reservedSize = 0; // The size reserved by the code buffer
    uintptr_t autoGrowSize = 64 * 1024; // How much the code buffer will be grown if AutoGrow is on and it overflows (default = 64KB)

    using BranchLabel = std::pair <uint32_t*, BranchType>;
    
    // Generic write function.
    // Used to implement write8, write16, write32, write64 and subsequently, db, dh, dw, and dd
    template <typename T>
    constexpr void write (T val) {
        if constexpr (growMode == GrowingMode::AutoGrow) {
            const auto currentSize = getCodeSize();

            if (currentSize + sizeof(T) >= reservedSize) { // if buffer will overflow, automatically grow it by 64KB
                const auto newSize = reservedSize + autoGrowSize; // The new reserved size for the code buffer
                const auto newBuffer = new uint32_t [newSize / 4]; // The new buffer
                std::memcpy (newBuffer, code, currentSize); // copy over the code into the new buffer
                printf ("Code buffer exceeded max size (Using: %X bytes. Allocated: %X bytes)\nResizing to %X\n", currentSize + 4, reservedSize, newSize);
                
                delete code; // delete old code buffer
                code = newBuffer; // set code to point to new buffer
                currentPointer = (uint32_t*) ((uintptr_t) newBuffer + currentSize); // Restore currentPointer's relative position
                reservedSize = newSize;
            }
        }

        auto tmp = (T*) currentPointer;
        *tmp++ = val;
        currentPointer = (uint32_t*) tmp;
    }

    constexpr void write8 (uint8_t val) { write <uint8_t> (val); }
    constexpr void write16 (uint16_t val) { write <uint16_t> (val); }
    constexpr void write32 (uint32_t val) { write <uint32_t> (val); }
    constexpr void write64 (uint64_t val) { write <uint64_t> (val); }

    template <typename T>
    void write (T* array, int size) {
        while (size--)
            write <T> (*array++);
    }

    constexpr BranchLabel emitBranch14 (uint32_t opcode) {
        const auto cia = currentPointer;
        write32 (opcode);
        return std::make_pair (cia, BranchType::Branch14);
    }

    static constexpr uint32_t signExtend32 (uint32_t value, uint32_t startingSize) {
        auto bitsToShift = 32 - startingSize;
        return (uint32_t) ((int32_t)value << bitsToShift >> bitsToShift);
    }

    [[noreturn]] static void panic(const char* fmt, ...) {
        std::va_list args;
        va_start(args, fmt);
        std::vprintf (fmt, args);
        va_end(args);

        while (1);
    }

    // Metaprogramming helpers for making a compile-time loop, used to implement the "repeat" directive
    // Many thanks to https://github.com/fleroviux
    template <typename T, T Begin,  class Func, T ...Is>
    constexpr void static_for_impl( Func &&f, std::integer_sequence<T, Is...> ) {
        ( f( std::integral_constant<T, Begin + Is>{ } ),... );
    }

    template <typename T, T Begin, T End, class Func >
    constexpr void static_for( Func &&f ) {
        static_for_impl <T, Begin>( std::forward<Func>(f), std::make_integer_sequence<T, End - Begin>{ } );
    }

public:
    PPCEmitter (uintptr_t bufferSize = 64 * 1024) { // Make an emitter object with "bufferSize" bytes of executable memory
        reservedSize = bufferSize;
        if (bufferSize == 0) // If bufferSize = 0 then skip allocating memory, because the user (hopefully) wants to allocate it themselves
            return;

        if (bufferSize & 3)
            panic ("[Emitter] Fatal: Buffer size is not word-aligned");

        code = new uint32_t [bufferSize / 4]; // TODO: This works on Wii but not on platforms where executable perms matter!!
        currentPointer = code;
        
        if (!code)
            panic ("[Emitter] Fatal: Failed to allocate memory\n");
    }

    uint32_t* getBuffer() { 
        return code; 
    }

    void setBuffer (uint32_t* pointer, uintptr_t bufferSize) { // Note: This completely wrecks the JIT cache and should only be used before actually emitting code
        code = pointer;
        currentPointer = pointer;
        reservedSize = bufferSize;

        if (bufferSize & 3)
            panic ("[Emitter] Fatal: Buffer size is not word-aligned");
    }

    constexpr uint32_t* getCurr() { 
        return currentPointer; 
    }

    uintptr_t getCodeSize() {
        return (uintptr_t) currentPointer - (uintptr_t) code;
    }

    // Sets how much the buffer will be grown if autogrow is on and the buffer overflows
    void setAutoGrowSize (uintptr_t size) {
        autoGrowSize = size;
    
        if (size & 3)
            panic ("[Emitter] Fatal: AutoGrow size is not word-aligned");
    }

    constexpr void db   (uint8_t val)  { write8 (val); } // Data byte
    constexpr void dh   (uint16_t val) { write16 (val); } // Data halfword
    constexpr void dw   (uint32_t val) { write32 (val); } // Data word
    constexpr void dd   (uint64_t val) { write64 (val); } // Data doubleword
    constexpr void df32 (float val )   { write <float> (val); } // Data float 32 (Single)
    constexpr void df64 (double val)   { write <double> (val); } // Data float 64 (Double)

    constexpr void db   (uint8_t* arr, int size)  { write <uint8_t> (arr, size); } //  Data byte array
    constexpr void dh   (uint16_t* arr, int size) { write <uint16_t> (arr, size); } //  Data halfword array
    constexpr void dw   (uint32_t* arr, int size) { write <uint32_t> (arr, size); } //  Data word array
    constexpr void dd   (uint64_t* arr, int size) { write <uint64_t> (arr, size); } //  Data doubleword array
    constexpr void df32 (float* arr, int size)    { write <float> (arr, size); } //  Data float array
    constexpr void df64 (double* arr, int size)   { write <double> (arr, size); } //  Data double array

    constexpr void ds (const char* str) { // Data string (null-terminated)
        while (*str != '\0') {// copy characters until null terminator
            db (*str++);
        }

        db ('\0'); // copy null terminator
    }

    void ds (std::string str) {
        ds (str.c_str());
    }

    void align (int bytes) {
        if (bytes == 1) 
            return;
        
        else if (bytes < 1)
            panic ("[Emitter] Fatal: Tried to align to a %d byte boundary", bytes);

        auto remainder = (uintptr_t) getCurr() % bytes;
        while (remainder--) 
            write8 (0);
    }

    template <size_t end, class Func>
    constexpr void repeat (Func&& f) {
        static_for_impl <size_t, 0>( std::forward<Func>(f), std::make_integer_sequence<size_t, end>{ } );
    }

    template <class Func>
    constexpr void loop (GPR counter, size_t iterations, Func&& f) {
        if (iterations == 0) return;         // Do nothing if 0 iterations

        liw (counter, iterations);           // load iterations into counter register
        const auto label = getCurr();        // Label for loop
        f();
        addic <true> (counter, counter, -1); // Decrement counter (need addic. because addi doesn't affect CR)
        const auto slot = bne();             // Loop if not 0
        setLabel (slot, label);
    }

    constexpr void ud() { write32 (0); } // Undefined opcode (use for debugging)
    void nop() { ori (r0, r0, 0); } // No operation

    void blr() { write32 (0x4E800020); } // Branch to link register
    void bctr() { write32 (0x4E800420); } // Branch to counter register
    void bctrl() { write32 (0x4E800421); } // Branch to counter register and link

    void li (GPR reg, int16_t imm) { // Load immediate (signed)
        addi (reg, r0, imm);
    }

    void liu (GPR reg, uint16_t imm) { // Load immediate (unsigned)
    	if (imm < 0x8000) // For immediates < 0x8000, we can use a single addi
    		li (reg, imm);
    	else {
    		li (reg, 0); // set register to 0.
    		ori (reg, reg, imm); // or register with immediate
    	}
    }

    void setz (GPR dest, GPR src) { // Set dest to 1 if src is 0, otherwise set dest to 0
        cntlzw (dest, src); // cntlzw returns 0-31 normally, but it returns 32 for 0. This means we can use bit 5 to check whether or not src is 0
        srwi (dest, dest, 5); // Shift bit 5 to LSB
    }

    void lis (GPR reg, uint16_t imm) {
        addis (reg, r0, imm);
    }

    template <bool setFlags = false>
    void nand (GPR dest, GPR src1, GPR src2) {
        write32 (0x7C0003B8 | (src1 << 21) | (dest << 16) | (src2 << 11) | setFlags);
    }

    template <bool setFlags = false>
    void and_ (GPR dest, GPR src1, GPR src2) {
        write32 (0x7C000038 | (src1 << 21) | (dest << 16) | (src2 << 11) | setFlags);
    }

    template <bool setFlags = false>
    void andc (GPR dest, GPR src1, GPR src2) {
        write32 (0x7C000078 | (src1 << 21) | (dest << 16) | (src2 << 11) | setFlags);
    }

    void andi (GPR dest, GPR src, uint16_t imm) {
        write32 (0x70000000 | (src << 21) | (dest << 16) | imm);
    }

    void andis (GPR dest, GPR src, uint16_t imm) {
        write32 (0x74000000 | (src << 21) | (dest << 16) | imm);
    }

    template <bool setFlags = false>
    void nor (GPR dest, GPR src1, GPR src2) {
        write32 (0x7C0000F8 | (src1 << 21) | (dest << 16) | (src2 << 11) | setFlags);
    }

    template <bool setFlags = false>
    void or_ (GPR dest, GPR src1, GPR src2) {
        write32 (0x7C000378 | (src1 << 21) | (dest << 16) | (src2 << 11) | setFlags);
    }

    template <bool setFlags = false>
    void orc (GPR dest, GPR src1, GPR src2) {
        write32 (0x7C000338 | (src1 << 21) | (dest << 16) | (src2 << 11) | setFlags);
    }

    void ori (GPR dest, GPR src, uint16_t imm) {
        write32 (0x60000000 | (src << 21) | (dest << 16) | imm);
    }

    void oris (GPR dest, GPR src, uint16_t imm) {
        write32 (0x64000000 | (src << 21) | (dest << 16) | imm);
    }

    template <bool setFlags = false>
    void xor_ (GPR dest, GPR src1, GPR src2) {
        write32 (0x7C000278 | (src1 << 21) | (dest << 16) | (src2 << 11) | setFlags);
    }

    void xori (GPR dest, GPR src, uint16_t imm) {
        write32 (0x68000000 | (src << 21) | (dest << 16) | imm);
    }

    void xoris (GPR dest, GPR src, uint16_t imm) {
        write32 (0x6C000000 | (src << 21) | (dest << 16) | imm);
    }

    template <bool setFlags = false>
    void add (GPR dest, GPR src1, GPR src2) {
        write32 (0x7C000214 | (dest << 21) | (src1 << 16) | (src2 << 11) | setFlags);
    }

    template <bool setFlags = false>
    void addo (GPR dest, GPR src1, GPR src2) {
        write32 (0x7C000614 | (dest << 21) | (src1 << 16) | (src2 << 11) | setFlags);
    }

    template <bool setFlags = false>
    void addc (GPR dest, GPR src1, GPR src2) {
        write32 (0x7C000014 | (dest << 21) | (src1 << 16) | (src2 << 11) | setFlags);
    }

    template <bool setFlags = false>
    void addco (GPR dest, GPR src1, GPR src2) {
        write32 (0x7C000414 | (dest << 21) | (src1 << 16) | (src2 << 11) | setFlags);
    }

    template <bool setFlags = false>
    void adde (GPR dest, GPR src1, GPR src2) {
        write32 (0x7C000114 | (dest << 21) | (src1 << 16) | (src2 << 11) | setFlags);
    }

    template <bool setFlags = false>
    void addeo (GPR dest, GPR src1, GPR src2) {
        write32 (0x7C000514 | (dest << 21) | (src1 << 16) | (src2 << 11) | setFlags);
    }

    template <bool setFlags = false>
    void addze (GPR dest, GPR src) {
        write32 (0x7C000194 | (dest << 21) | (src << 16) | setFlags);
    }

    template <bool setFlags = false>
    void addzeo (GPR dest, GPR src) {
        write32 (0x7C000594 | (dest << 21) | (src << 16) | setFlags);
    }

    void addi (GPR dest, GPR src, int16_t imm) {
        write32 (0x38000000 | (dest << 21) | (src << 16) | (uint16_t) imm);
    }

    void addis (GPR dest, GPR src, int16_t imm) {
        write32 (0x3C000000 | (dest << 21) | (src << 16) | (uint16_t) imm);
    }

    template <bool setFlags = false>
    void addic (GPR dest, GPR src, int16_t imm) {
        if (!setFlags) // addic
            write32 (0x30000000 | (dest << 21) | (src << 16) | (uint16_t) imm);
        else // addic.
            write32 (0x34000000 | (dest << 21) | (src << 16) | (uint16_t) imm);
    }

    template <bool setFlags = false>
    void addme (GPR dest, GPR src) {
        write32 (0x7C0001D4 | (dest << 21) | (src << 16) | setFlags);
    }

    template <bool setFlags = false>
    void addmeo (GPR dest, GPR src) {
        write32 (0x7C0005D4 | (dest << 21) | (src << 16) | setFlags);
    }

    template <bool setFlags = false>
    void subf (GPR dest, GPR src1, GPR src2) {
        write32 (0x7C000050 | (dest << 21) | (src1 << 16) | (src2 << 11) | setFlags);
    }

    template <bool setFlags = false>
    void sub (GPR dest, GPR src1, GPR src2) { // subf except without reversed operands
        subf <setFlags> (dest, src2, src1);
    }

    template <bool setFlags = false>
    void subfo (GPR dest, GPR src1, GPR src2) {
        write32 (0x7C000450 | (dest << 21) | (src1 << 16) | (src2 << 11) | setFlags);
    }

    template <bool setFlags = false>
    void subo (GPR dest, GPR src1, GPR src2) { // subfo except without reversed operands
        subfo <setFlags> (dest, src2, src1);
    }

    template <bool setFlags = false>
    void subfc (GPR dest, GPR src1, GPR src2) {
        write32 (0x7C000010 | (dest << 21) | (src1 << 16) | (src2 << 11) | setFlags);
    }

    template <bool setFlags = false>
    void subc (GPR dest, GPR src1, GPR src2) { // subfc without reversed operands
        subfc <setFlags> (dest, src2, src1);
    }

    template <bool setFlags = false>
    void subfco (GPR dest, GPR src1, GPR src2) {
        write32 (0x7C000410 | (dest << 21) | (src1 << 16) | (src2 << 11) | setFlags);
    }

    template <bool setFlags = false>
    void subco (GPR dest, GPR src1, GPR src2) { // subco without reversed operands
        subfco <setFlags> (dest, src2, src1);
    }

    template <bool setFlags = false>
    void subfe (GPR dest, GPR src1, GPR src2) {
        write32 (0x7C000110 | (dest << 21) | (src1 << 16) | (src2 << 11) | setFlags);
    }

    template <bool setFlags = false>
    void sube (GPR dest, GPR src1, GPR src2) { // subfe without reversed operands
        subfe <setFlags> (dest, src2, src1);
    }

    template <bool setFlags = false>
    void subfeo (GPR dest, GPR src1, GPR src2) {
        write32 (0x7C000510 | (dest << 21) | (src1 << 16) | (src2 << 11) | setFlags);
    }

    template <bool setFlags = false>
    void subeo (GPR dest, GPR src1, GPR src2) { // subfeo without reversed operands
        subfeo <setFlags> (dest, src2, src1);
    }

    void subfic (GPR dest, GPR src, int16_t imm) {
        write32 (0x20000000 | (dest << 21) | (src << 16) | (uint16_t) imm);
    }

    template <bool setFlags = false>
    void subfme (GPR dest, GPR src) {
        write32 (0x7C0001D0 | (dest << 21) | (src << 16) | setFlags);   
    }

    template <bool setFlags = false>
    void subfmeo (GPR dest, GPR src) {
        write32 (0x7C0005D0 | (dest << 21) | (src << 16) | setFlags);   
    }

    template <bool setFlags = false>
    void subfze (GPR dest, GPR src) {
        write32 (0x7C000190 | (dest << 21) | (src << 16) | setFlags);   
    }

    template <bool setFlags = false>
    void subfzeo (GPR dest, GPR src) {
        write32 (0x7C000590 | (dest << 21) | (src << 16) | setFlags);   
    }
    
    void cmpli (CR dest, GPR src, uint16_t imm) {
        write32 (0x28000000 | (dest << 23) | (src << 16) | imm);
    }

    void cmpi (CR dest, GPR src, int16_t imm) {
        write32 (0x2C000000 | (dest << 23) | (src << 16) | (uint16_t) imm);
    }

    void cmpl (CR dest, GPR src1, GPR src2) {
        write32 (0x7C000040 | (dest << 23) | (src1 << 16) | (src2 << 11));
    }
    
    void mulli (GPR dest, GPR src, int16_t imm) {
        write32 (0x1C000000 | (dest << 21) | (src << 16) | (uint16_t) imm);
    }

    template <bool setFlags = false>
    void mullw (GPR dest, GPR src1, GPR src2) {
        write32 (0x7C0001D6 | (dest << 21) | (src1 << 16) | (src2 << 11) | setFlags);
    }

    template <bool setFlags = false>
    void mullwo (GPR dest, GPR src1, GPR src2) {
        write32 (0x7C0003D6 | (dest << 21) | (src1 << 16) | (src2 << 11) | setFlags);
    }

    template <bool setFlags = false>
    void mulhw (GPR dest, GPR src1, GPR src2) {
        write32 (0x7C000096 | (dest << 21) | (src1 << 16) | (src2 << 11) | setFlags);
    }

    template <bool setFlags = false>
    void mulhwu (GPR dest, GPR src1, GPR src2) {
        write32 (0x7C000016 | (dest << 21) | (src1 << 16) | (src2 << 11) | setFlags);
    }

    template <bool setFlags = false>
    void divwu (GPR dest, GPR dividend, GPR divisor) {
        write32 (0x7C000396 | (dest << 21) | (dividend << 16) | (divisor << 11) | setFlags);
    }

    template <bool setFlags = false>
    void divwuo (GPR dest, GPR dividend, GPR divisor) {
        write32 (0x7C000796 | (dest << 21) | (dividend << 16) | (divisor << 11) | setFlags);
    }

    template <bool setFlags = false>
    void divw (GPR dest, GPR dividend, GPR divisor) {
        write32 (0x7C0003D6 | (dest << 21) | (dividend << 16) | (divisor << 11) | setFlags);
    }

    template <bool setFlags = false>
    void divwo (GPR dest, GPR dividend, GPR divisor) {
        write32 (0x7C0007D6 | (dest << 21) | (dividend << 16) | (divisor << 11) | setFlags);
    }
    
    template <bool setFlags = false>
    void mr (GPR dest, GPR src) { // Move register
        or_ <setFlags> (dest, src, src);
    }

    void liw (GPR reg, uint32_t imm) {
        if (imm <= 0x7FFF || imm >= 0xFFFF8000) // use li if possible
            li (reg, imm);

        else if ((imm & 0xFFFF) == 0) // If lower halfword is 0 -> use lis
            lis (reg, imm >> 16);

        else { // else: use lis + ori
            lis (reg, imm >> 16);
            ori (reg, reg, (uint16_t) imm);
        }
    }

    template <bool setFlags = false>
    void slw (GPR dest, GPR src1, GPR src2) { // Shift left word
        write32 (0x7C000030 | (src1 << 21) | (dest << 16) | (src2 << 11) | setFlags);
    }

    template <bool setFlags = false>
    void srw (GPR dest, GPR src1, GPR src2) { // Shift right word
        write32 (0x7C000430 | (src1 << 21) | (dest << 16) | (src2 << 11) | setFlags);
    }

    template <bool setFlags = false>
    void sraw (GPR dest, GPR src1, GPR src2) { // Shift right algebraic word
        write32 (0x7C000630 | (src1 << 21) | (dest << 16) | (src2 << 11) | setFlags);
    }

    template <bool setFlags = false>
    void srawi (GPR dest, GPR src, uint8_t amount) { // Shift left right algebraic word immediate
        write32 (0x7C000670 | (src << 21) | (dest << 16) | (amount << 11) | setFlags);
    }
    
    template <bool setFlags = false>
    void rlwinm (GPR dest, GPR src, uint8_t shift, uint8_t mb, uint8_t me) { // Rotate left word then and 
        write32 (0x54000000 | (src << 21) | (dest << 16) | ((shift & 31) << 11) | (mb << 6) | (me << 1) | setFlags);
    }

    template <bool setFlags = false>
    void slwi (GPR dest, GPR src, uint8_t shift) { // shift left word by immediate
        rlwinm <setFlags> (dest, src, shift, 0, 31 - shift); // SLWI is actually just an alias for an rlwinm configuration
    }

    template <bool setFlags = false>
    void srwi (GPR dest, GPR src, uint8_t shift) { // shift right word by immediate
        rlwinm <setFlags> (dest, src, 32 - shift, shift, 31); // SRWI is actually just an alias for an rlwinm configuration
    }

    template <bool setFlags = false>
    void clrlwi (GPR dest, GPR src, uint8_t len) { // Bit clear left
        rlwinm <setFlags> (dest, src, 0, len, 31); // Very intuitively, this is also a rlwinm alias
    }

    template <bool setFlags = false>
    void clrrwi (GPR dest, GPR src, uint8_t len) { // Bit clear right
        rlwinm <setFlags> (dest, src, 0, 0, 31 - len); // Very intuitively, this is also a rlwinm alias
    }

    template <bool setFlags = false>
    void rotlwi (GPR dest, GPR src, uint8_t amount) { // Rotate left word immediate
        rlwinm <setFlags> (dest, src, amount, 0, 31); // This architecture has a lot of rlwinm aliases
    }

    template <bool setFlags = false>
    void rotrwi (GPR dest, GPR src, uint8_t amount) { // Rotate left right immediate
        rlwinm <setFlags> (dest, src, 32 - amount, 0, 31); // This architecture has a lot of rlwinm aliases
    }

    template <bool setFlags = false>
    void extlwi (GPR dest, GPR src, uint8_t n, uint8_t b) { // Extract and left justify immediate
        rlwinm <setFlags> (dest, src, b, 0, n - 1); // Yet another rlwinm alias
    }

    template <bool setFlags = false>
    void extrwi (GPR dest, GPR src, uint8_t n, uint8_t b) { // Extract and left justify immediate
        rlwinm <setFlags> (dest, src, b + n, 32 - n, 31); // Did you know rlwinm is useful?
    }

    template <bool setFlags = false>
    void rlwnm (GPR dest, GPR src, GPR amount, uint8_t mb, uint8_t me) { // Rotate left word then AND with mask
        write32 (0x5C000000 | (src << 21) | (dest << 16) | (amount << 11) | (mb << 6) | (me << 1) | setFlags);
    }

    template <bool setFlags = false>
    void rlwimi (GPR dest, GPR src, uint8_t shift, int mb, int me) { // Rotate left word immediate then mask insert
        write32 (0x50000000 | (src << 21) | (dest << 16) | (shift << 11) | (mb << 6) | (me << 1) | setFlags);
    }

    template <bool setFlags = false>
    void cntlzw (GPR dest, GPR src) { // Count Leading Zeroes Word
        write32 (0x7C000034 | (src << 21) | (dest << 16));
    }

    void stb (GPR src, GPR base, int16_t offset) { // Store byte    Note: Source first!
        write32 (0x98000000 | (src << 21) | (base << 16) | (uint16_t) offset);
    }

    void stbx (GPR src, GPR index, GPR base) { // Store byte indexed   Note: Source first!
        write32 (0x7C0001AE | (src << 21) | (index << 16) | (base << 11));
    }

    void stbu (GPR src, GPR base, int16_t offset) { // Store byte with update (writeback)    Note: Source first!
        write32 (0x9C000000 | (src << 21) | (base << 16) | (uint16_t) offset);
    }

    void stbux (GPR src, GPR index, GPR base) { // Store byte with update (writeback) indexed   Note: Source first!
        write32 (0x7C0001EE | (src << 21) | (index << 16) | (base << 11));
    }

    void sth (GPR src, GPR base, int16_t offset) { // Store halfword    Note: Source first!
        write32 (0xB0000000 | (src << 21) | (base << 16) | (uint16_t) offset);
    }

    void sthx (GPR src, GPR index, GPR base) { // Store halfword indexed   Note: Source first!
        write32 (0x7C00032E | (src << 21) | (index << 16) | (base << 11));
    }

    void sthu (GPR src, GPR base, int16_t offset) { // Store halfword with update (writeback)    Note: Source first!
        write32 (0xB4000000 | (src << 21) | (base << 16) | (uint16_t) offset);
    }

    void sthux (GPR src, GPR index, GPR base) { // Store halfword with update (writeback) indexed   Note: Source first!
        write32 (0x7C00036E | (src << 21) | (index << 16) | (base << 11));
    }

    void stw (GPR src, GPR base, int16_t offset) { // Store word    Note: Source first!
        write32 (0x90000000 | (src << 21) | (base << 16) | (uint16_t) offset);
    }

    void stwx (GPR src, GPR index, GPR base) { // Store word indexed   Note: Source first!
        write32 (0x7C00012E | (src << 21) | (index << 16) | (base << 11));
    }

    void stwu (GPR src, GPR base, int16_t offset) { // Store word with update (writeback)   Note: Source first!
        write32 (0x94000000 | (src << 21) | (base << 16) | (uint16_t) offset);
    }

    void stwux (GPR src, GPR index, GPR base) { // Store word with update indexed   Note: Source first!
        write32 (0x7C00016E | (src << 21) | (index << 16) | (base << 11));
    }

    void lbz (GPR dest, GPR base, int16_t offset) { // Load byte and zero
        write32 (0x88000000 | (dest << 21) | (base << 16) | (uint16_t) offset);
    }

    void lbzx (GPR dest, GPR index, GPR base) { // Load byte and zero indexed
        write32 (0x7C0000AE | (dest << 21) | (index << 16) | (base << 11));
    }

    void lbzu (GPR dest, GPR base, int16_t offset) { // Load byte and zero with update (writeback)
        write32 (0x8C000000 | (dest << 21) | (base << 16) | (uint16_t) offset);
    }

    void lbzux (GPR dest, GPR index, GPR base) { // Load byte and zero with update indexed
        write32 (0x7C0000EE | (dest << 21) | (index << 16) | (base << 11));
    }

    void lhz (GPR dest, GPR base, int16_t offset) { // Load halfword and zero
        write32 (0xA0000000 | (dest << 21) | (base << 16) | (uint16_t) offset);
    }

    void lhzx (GPR dest, GPR index, GPR base) { // Load halfword and zero indexed
        write32 (0x7C00022E | (dest << 21) | (index << 16) | (base << 11));
    }

    void lhzu (GPR dest, GPR base, int16_t offset) { // Load halfword and zero with update (writeback)
        write32 (0xA4000000 | (dest << 21) | (base << 16) | (uint16_t) offset);
    }

    void lhzux (GPR dest, GPR index, GPR base) { // Load halfword and zero with update indexed
        write32 (0x7C00026E | (dest << 21) | (index << 16) | (base << 11));
    }

    void lhax (GPR dest, GPR index, GPR base) { // Load halfword algebraic indexed
        write32 (0x7C0002AE | (dest << 21) | (index << 16) | (base << 11));
    }

    void lhaux (GPR dest, GPR index, GPR base) { // Load halfword algebraic with update indexed
        write32 (0x7C0002EE | (dest << 21) | (index << 16) | (base << 11));
    }

    void lhbrx (GPR dest, GPR index, GPR base) { // Load halfword byte-reverse indexed
        write32 (0x7C00062C | (dest << 21) | (index << 16) | (base << 11));
    }

    void lwz (GPR dest, GPR base, int16_t offset) { // Load word and zero
        write32 (0x80000000 | (dest << 21) | (base << 16) | (uint16_t) offset);
    }

    void lwzx (GPR dest, GPR index, GPR base) { // Load word and zero with update indexed
        write32 (0x7C00002E | (dest << 21) | (index << 16) | (base << 11));
    }

    void lwzu (GPR dest, GPR base, int16_t offset) { // Load word and zero with update (writeback)
        write32 (0x84000000 | (dest << 21) | (base << 16) | (uint16_t) offset);
    }

    void lwzux (GPR dest, GPR index, GPR base) { // Load word and zero with update indexed
        write32 (0x7C00006E | (dest << 21) | (index << 16) | (base << 11));
    }

    void lwarx (GPR dest, GPR index, GPR base) { // Load word and Reserve indexed
        write32 (0x7C000028 | (dest << 21) | (index << 16) | (base << 11));
    }

    void lwbrx (GPR dest, GPR index, GPR base) { // Load word byte-reverse indexed
        write32 (0x7C00042C | (dest << 21) | (index << 16) | (base << 11));
    }

    void lmw (GPR dest, GPR base, int16_t offset) { // Load multiple words
        write32 (0xB8000000 | (dest << 21) | (base << 16) | (uint16_t) offset);
    }

    void stmw (GPR src, GPR base, int16_t offset) { // Store multiple words
        write32 (0xBC000000 | (src << 21) | (base << 16) | (uint16_t) offset);
    }

    template <bool link>
    BranchLabel bx() {
        const auto cia = currentPointer;
        write32 (0x48000000 | link);
        return std::make_pair (cia, BranchType::Branch24);
    }

    BranchLabel b() {
        return bx <false> ();
    }

    BranchLabel bl() {
        return bx <true> ();
    } 

    template <bool link>
    void bx (void* address) {
        const auto cia = (uintptr_t)currentPointer; // Load CIA
        const intptr_t disp = (intptr_t) address - (intptr_t) cia; // Displacement of the jump in bytes
                
        constexpr int32_t INT26_MIN = -0x2000000; // Bx uses an INT26_t displacement, so we define a ceiling and a floor
        constexpr int32_t INT26_MAX = 0x1FFFFFF;

        if ((uintptr_t) address & 3) // Check that address is aligned to a word boundary
            panic ("[Emitter] Fatal: Unaligned branch displacement");
        
        if (disp >= INT26_MIN && disp <= INT26_MAX) // Check if the displacement in words can be encoded in 24 bits in a relative branch
            write32 (0x48000000 | (disp & 0x3FFFFFC) | link);
        else if ((intptr_t) address >= INT26_MIN && (intptr_t) address <= INT26_MAX) // Check if the target address can be encoded in 24 bits in an absolute branch instead
            write32 (0x48000000 | ((uintptr_t) address & 0x3FFFFFC) | 2 | link);
        else
            panic ("[Emitter] Fatal: Invalid label for 24-bit branch, displacement of %08X words exceeds possible range\n", disp >> 2);
    }

    void b (void* address) {
        bx <false> (address);
    }
    
    void bl (void* address) {
        bx <true> (address);
    }

    // TODO: Perhaps handle conditional branches targetting CR1-7
    template <Cond cond, bool link>
    BranchLabel bcx () {
        constexpr bool shouldBitBeSet = (uint32_t) cond <= 3; // For conditions 0 (Lt), 1 (Gt), 2 (Eq) and 3 (SO), the bit in CR should be set. Otherwise it should be cleared
        constexpr int bit = (uint32_t) cond & 3;

        return emitBranch14 (0x40800000 | (shouldBitBeSet << 24) | (bit << 16) | link);
    }

    BranchLabel beq() { return bcx <Cond::Eq, false> (); } // Branch if equal
    BranchLabel bne() { return bcx <Cond::Ne, false> (); } // Branch if not equal
    BranchLabel blt() { return bcx <Cond::Lt, false> (); } // Branch if less than
    BranchLabel bge() { return bcx <Cond::Ge, false> (); } // Branch if greater than or equal
    BranchLabel ble() { return bcx <Cond::Le, false> (); } // Branch if less than or equal
    BranchLabel bgt() { return bcx <Cond::Gt, false> (); } // Branch if greater than
    BranchLabel bso() { return bcx <Cond::Os, false> (); } // Branch if overflow
    BranchLabel bns() { return bcx <Cond::Oc, false> (); } // Branch if no overflow

    BranchLabel beql() { return bcx <Cond::Eq, true> (); } // Branch if equal and link
    BranchLabel bnel() { return bcx <Cond::Ne, true> (); } // Branch if not equal and link
    BranchLabel bltl() { return bcx <Cond::Lt, true> (); } // Branch if less than and link
    BranchLabel bgel() { return bcx <Cond::Ge, true> (); } // Branch if greater than or equal and link
    BranchLabel blel() { return bcx <Cond::Le, true> (); } // Branch if less than or equal and link
    BranchLabel bgtl() { return bcx <Cond::Gt, true> (); } // Branch if greater than and link
    BranchLabel bsol() { return bcx <Cond::Os, true> (); } // Branch if overflow and link
    BranchLabel bnsl() { return bcx <Cond::Oc, true> (); } // Branch if no overflow and link

    void setLabel (BranchLabel label) {
        setLabel (label, currentPointer);
    }

    void setLabel (BranchLabel label, void* address) {
        const auto instrAddress = label.first; // Load the address of the instruction to patch
        const auto type = label.second; // Type of branch
        const intptr_t disp = (intptr_t) address - (intptr_t) instrAddress; // Displacement of the jump in bytes

        constexpr int32_t INT26_MIN = -0x2000000;
        constexpr int32_t INT26_MAX = 0x1FFFFFF;

        if (disp & 3)
            panic ("[Fatal] Unaligned branch displacement\n");

        switch (type) {
            case BranchType::Branch14: {
                if (disp <= INT16_MAX && disp >= INT16_MIN) // Check if the displacement in words can be encoded in 14 bits in a relative branch
                    *instrAddress = (*instrAddress & ~0xFFFE) | (disp & 0xFFFC);
                else if ((intptr_t) address <= INT16_MAX && (intptr_t) address >= INT16_MIN) // Check if the target address can be encoded in 14 bits in an absolute branch instead
                    *instrAddress = (*instrAddress & ~0xFFFE) | ((uintptr_t)address & 0xFFFC) | 2;
                else
                    panic ("Invalid label for 14-bit branch, displacement of %08X words exceeds possible range\n", disp >> 2);
                break;
            }
            
            case BranchType::Branch24: {
                if (disp >= INT26_MIN && disp <= INT26_MAX) // Check if the displacement in words can be encoded in 24 bits in a relative branch
                    *instrAddress = (*instrAddress & ~0x3FFFFFE) | (disp & 0x3FFFFFC);
                else if ((intptr_t) address >= INT26_MIN && (intptr_t) address <= INT26_MAX) // Check if the target address can be encoded in 24 bits in an absolute branch instead
                    *instrAddress = (*instrAddress & ~0x3FFFFFE) | ((uintptr_t)address & 0x3FFFFFC) | 2;
                else
                    panic ("Invalid label for 24-bit branch, displacement of %08X words exceeds possible range\n", disp >> 2);
                break;
            }
        }
    }

    // CR/MSR/SPR/FPSCR/SR operations

    void crand (uint8_t dest_bit, uint8_t src1_bit, uint8_t src2_bit) { // Condition register AND
        write32 (0x4C000202 | (dest_bit << 21) | (src1_bit << 16) | (src2_bit << 11));
    }
    
    void crandc (uint8_t dest_bit, uint8_t src1_bit, uint8_t src2_bit) { // Condition register AND with complement
        write32 (0x4C000102 | (dest_bit << 21) | (src1_bit << 16) | (src2_bit << 11));
    }

    void creqv (uint8_t dest_bit, uint8_t src1_bit, uint8_t src2_bit) { // Condition register XNOR
        write32 (0x4C000242 | (dest_bit << 21) | (src1_bit << 16) | (src2_bit << 11));
    }

    void crnand (uint8_t dest_bit, uint8_t src1_bit, uint8_t src2_bit) { // Condition register NAND
        write32 (0x4C0001C2 | (dest_bit << 21) | (src1_bit << 16) | (src2_bit << 11));
    }

    void crnor (uint8_t dest_bit, uint8_t src1_bit, uint8_t src2_bit) { // Condition register NOR
        write32 (0x4C000042 | (dest_bit << 21) | (src1_bit << 16) | (src2_bit << 11));
    }

    void cror (uint8_t dest_bit, uint8_t src1_bit, uint8_t src2_bit) { // Condition register OR
        write32 (0x4C000382 | (dest_bit << 21) | (src1_bit << 16) | (src2_bit << 11));
    }

    void crorc (uint8_t dest_bit, uint8_t src1_bit, uint8_t src2_bit) { // Condition register OR with complement
        write32 (0x4C000342 | (dest_bit << 21) | (src1_bit << 16) | (src2_bit << 11));
    }

    void crxor (uint8_t dest_bit, uint8_t src1_bit, uint8_t src2_bit) { // Condition register xor
        write32 (0x4C000182 | (dest_bit << 21) | (src1_bit << 16) | (src2_bit << 11));
    }

    void mtcrf (uint8_t mask, GPR src) { // Move to condition register fields
        write32 (0x7C000120 | (src << 21) | (mask << 12));
    }

    void mtcr (GPR src) { mtcrf (0xFF, src); } // Move to condition register

    void mfcr (GPR dest) {
        write32 (0x7C000026 | (dest << 21));
    }

    void mtsr (SR dest, GPR src) { // Move to segment register
        write32 (0x7C0001A4 | (src << 21) | (dest << 16));
    }

    void mfsr (GPR dest, SR src) { // Move from segment register
        write32 (0x7C0004A6 | (dest << 21) | (src << 16));
    }

    void mtsrin (GPR src, GPR base) { // Move to segment register indirect
        write32 (0x7C0001E4 | (src << 21) | (base << 11));
    }

    void mfsrin (GPR dest, GPR base) { // Move to segment register indirect
        write32 (0x7C000526 | (dest << 21) | (base << 11));
    }

    void mfmsr (GPR dest) { // Move from machine state register
        write32 (0x7C0000A6 | (dest << 21));
    }

    void mtmsr (GPR src) { // Move to machine state register
        write32 (0x7C000124 | (src << 21));
    }

    void mtctr (GPR reg) { // Move to counter register
        write32 (0x7C0903A6 | (reg << 21));
    }

    void mfctr (GPR reg) { // Move from counter register
        write32 (0x7C0902A6 | (reg << 21));
    }

    void mflr (GPR dest) { // Move from link register
        write32 (0x7C0802A6 | (dest << 21));
    }

    void mtlr (GPR src) { // Move to link register
        write32 (0x7C0803A6 | (src << 21));
    }

    // FPU operations

    void lfs (FPR dest, GPR base, int16_t offset) { // Load floating point single
        write32 (0xC0000000 | (dest << 21) | (base << 16) | (uint16_t) offset);
    }

    void lfd (FPR dest, GPR base, int16_t offset) { // Load floating point double
        write32 (0xC8000000 | (dest << 21) | (base << 16) | (uint16_t) offset);
    }

    void stfs (FPR src, GPR base, int16_t offset) { // Store floating point single
        write32 (0xD0000000 | (src << 21) | (base << 16) | (uint16_t) offset);
    }

    void stfd (FPR src, GPR base, int16_t offset) { // Store floating point double
        write32 (0xD8000000 | (src << 21) | (base << 16) | (uint16_t) offset);
    }

    template <bool setFlags = false>
    void fmr (FPR dest, FPR src) { // Floating point move register
        write32 (0xFC000090 | (dest << 21) | (src << 11) | setFlags);
    }

    template <bool setFlags = false>
    void fadd (FPR dest, FPR src1, FPR src2) { // Floating Add Double
        write32 (0xFC00002A | (dest << 21) | (src1 << 16) | (src2 << 11) | setFlags);
    }

    template <bool setFlags = false>
    void fadds (FPR dest, FPR src1, FPR src2) { // Floating Add Single
        write32 (0xEC00002A | (dest << 21) | (src1 << 16) | (src2 << 11) | setFlags);
    }

    template <bool setFlags = false>
    void fdiv (FPR dest, FPR src1, FPR src2) { // Floating Add Double
        write32 (0xFC000024 | (dest << 21) | (src1 << 16) | (src2 << 11) | setFlags);
    }

    template <bool setFlags = false>
    void fdivs (FPR dest, FPR src1, FPR src2) { // Floating Add Double
        write32 (0xEC000024 | (dest << 21) | (src1 << 16) | (src2 << 11) | setFlags);
    }

    template <bool setFlags = false>
    void fmadd (FPR dest, FPR src1, FPR src2, FPR src3) { // Floating multiply-add double (dest = src 1 * src2 + src3)
        write32 (0xFC00003A | (dest << 21) | (src1 << 16) | (src3 << 11) | (src2 << 6) | setFlags);
    }

    template <bool setFlags = false>
    void fmadds (FPR dest, FPR src1, FPR src2, FPR src3) { // Floating multiply-add single (dest = src 1 * src2 + src3)
        write32 (0xEC00003A | (dest << 21) | (src1 << 16) | (src3 << 11) | (src2 << 6) | setFlags);
    }

    template <bool setFlags = false>
    void fmsub (FPR dest, FPR src1, FPR src2, FPR src3) { // Floating multiply-sub double (dest = src 1 * src2 - src3)
        write32 (0xFC000038 | (dest << 21) | (src1 << 16) | (src3 << 11) | (src2 << 6) | setFlags);
    }

    template <bool setFlags = false>
    void fmsubs (FPR dest, FPR src1, FPR src2, FPR src3) { // Floating multiply-sub single (dest = src 1 * src2 - src3)
        write32 (0xEC000038 | (dest << 21) | (src1 << 16) | (src3 << 11) | (src2 << 6) | setFlags);
    }

    template <bool setFlags = false>
    void fmul (FPR dest, FPR src1, FPR src2) { // Floating point multiply double
        write32 (0xFC000032 | (dest << 21) | (src1 << 16) | (src2 << 6) | setFlags);
    }

    template <bool setFlags = false>
    void fmuls (FPR dest, FPR src1, FPR src2) { // Floating point multiply single
        write32 (0xEC000032 | (dest << 21) | (src1 << 16) | (src2 << 6) | setFlags);
    }

    template <bool setFlags = false>
    void fnabs (FPR dest, FPR src) { // Floating point negative absolute value
        write32 (0xFC000110 | (dest << 21) | (src << 11) | setFlags);
    }

    template <bool setFlags = false> 
    void fneg (FPR dest, FPR src) { // Floating point negate
        write32 (0xFC000050 | (dest << 21) | (src << 11) | setFlags);
    }

    template <bool setFlags = false>
    void fnmadd (FPR dest, FPR src1, FPR src2, FPR src3) { // Floating negative multiply-add double (dest = -(src 1 * src2 + src3))
        write32 (0xFC00003E | (dest << 21) | (src1 << 16) | (src3 << 11) | (src2 << 6) | setFlags);
    }

    template <bool setFlags = false>
    void fnmadds (FPR dest, FPR src1, FPR src2, FPR src3) { // Floating negative multiply-add single (dest = -(src 1 * src2 + src3))
        write32 (0xEC00003E | (dest << 21) | (src1 << 16) | (src3 << 11) | (src2 << 6) | setFlags);
    }

    template <bool setFlags = false>
    void fnmsub (FPR dest, FPR src1, FPR src2, FPR src3) { // Floating negative multiply-subtract double (dest = -(src 1 * src2 - src3))
        write32 (0xFC00003C | (dest << 21) | (src1 << 16) | (src3 << 11) | (src2 << 6) | setFlags);
    }

    template <bool setFlags = false>
    void fnmsubs (FPR dest, FPR src1, FPR src2, FPR src3) { // Floating negative multiply-subtract single (dest = -(src 1 * src2 - src3))
        write32 (0xEC00003C | (dest << 21) | (src1 << 16) | (src3 << 11) | (src2 << 6) | setFlags);
    }

    template <bool setFlags = false> 
    void fres (FPR dest, FPR src) { // Floating reciprocal estimate single
        write32 (0xEC000030 | (dest << 21) | (src << 11) | setFlags);
    }

    template <bool setFlags = false> 
    void frsp (FPR dest, FPR src) { // Floating round to single
        write32 (0xFC000018 | (dest << 21) | (src << 11) | setFlags);
    }

    template <bool setFlags = false> 
    void frsqte (FPR dest, FPR src) { // Floating reciprocal square root estimate
        write32 (0xFC000034 | (dest << 21) | (src << 11) | setFlags);
    }

    template <bool setFlags = false>
    void fsel (FPR dest, FPR src1, FPR src2, FPR src3) { // Floating select
        write32 (0xFC00002E | (dest << 21) | (src1 << 16) | (src3 << 11) | (src2 << 6) | setFlags);
    }

    template <bool setFlags = false>
    void fsub (FPR dest, FPR src1, FPR src2) { // Floating Subtract Double
        write32 (0xFC000028 | (dest << 21) | (src1 << 16) | (src2 << 11) | setFlags);
    }

    template <bool setFlags = false>
    void fsubs (FPR dest, FPR src1, FPR src2) { // Floating Subtract Single
        write32 (0xEC000028 | (dest << 21) | (src1 << 16) | (src2 << 11) | setFlags);
    }

    // Useless shit
    void icbi (GPR rA, GPR rB) { // Instruction cache block invalidate
        write32 (0x7C0007AC | (rA << 16) | (rB << 11));
    }

    void dcbf (GPR rA, GPR rB) { // Data cache block flush
        write32 (0x7C0000AC | (rA << 16) | (rB << 11));
    }

    void dcbi (GPR rA, GPR rB) { // Data cache block invalidate
        write32 (0x7C0003AC | (rA << 16) | (rB << 11));
    }

    void dcbst (GPR rA, GPR rB) { // Data cache block store
        write32 (0x7C00006C | (rA << 16) | (rB << 11));
    }

    void dcbt (GPR rA, GPR rB) { // Data cache block touch
        write32 (0x7C00022C | (rA << 16) | (rB << 11));
    }

    void dcbtst (GPR rA, GPR rB) { // Data cache block touch for store
        write32 (0x7C0001EC | (rA << 16) | (rB << 11));
    }

    void dcbz (GPR rA, GPR rB) { // Data cache block clear to zero (Also known as: Thanos snap)
        write32 (0x7C0007EC | (rA << 16) | (rB << 11));
    }

    void dcbz_l (GPR rA, GPR rB) { // Data cache block clear to zero locked
        write32 (0x100007EC | (rA << 16) | (rB << 11));
    }

    void tlbie (GPR base) { // TLB invalidate entry
        write32 (0x7C000264 | (base << 11));
    }

    void tlbsync() { write32 (0x7C00046C); } // TLB synchronize
    void eieio() { write32 (0x7C0006AC); } // Enforce in-order execution of I/O
    void isync() { write32 (0x4C00012C); } // Instruction synchronize
    void sync()  { write32 (0x7C0004AC); } // Synchronize
    void rfi()   { write32 (0x4C000064); } // Return from interrupt
    void sc()    { write32 (0x44000002); } // System call

    void dump (std::string path) {
        const uint32_t size = getCodeSize();
        std::ofstream file (path, std::ios::binary);
        file.write ((const char*) code, size);
        printf ("Dumped %u bytes\n", size);
    }

    // System-specific extensions
    // IBM Gekko/Broadway SIMD instruction set (Paired Singles)
    template <bool setFlags = false>
    void ps_abs (FPR dest, FPR src) { // Paired single absolute value
        write32 (0x10000210 | (dest << 21) | (src << 11) | setFlags);
    }

    template <bool setFlags = false>
    void ps_add (FPR dest, FPR src1, FPR src2) { // Paired single add
        write32 (0x1000002A | (dest << 21) | (src1 << 16) | (src2 << 11) | setFlags);
    }

    void ps_cmpo0 (CR dest, FPR src1, FPR src2) { // Paired single compare ordered high
        write32 (0x10000040 | (dest << 23) | (src1 << 16) | (src2 << 11));
    }

    void ps_cmpo1 (CR dest, FPR src1, FPR src2) { // Paired single compare ordered low
        write32 (0x100000C0 | (dest << 23) | (src1 << 16) | (src2 << 11));
    }

    void ps_cmpu0 (CR dest, FPR src1, FPR src2) { // Paired single compare unordered high
        write32 (0x10000000 | (dest << 23) | (src1 << 16) | (src2 << 11));
    }

    void ps_cmpu1 (CR dest, FPR src1, FPR src2) { // Paired single compare unordered low
        write32 (0x10000080 | (dest << 23) | (src1 << 16) | (src2 << 11));
    }

    template <bool setFlags = false>
    void ps_div (FPR dest, FPR dividend, FPR divisor) { // Paired single divide
        write32 (0x10000024 | (dest << 21) | (dividend << 16) | (divisor << 11) | setFlags);
    }

    template <bool setFlags = false>
    void ps_madd (FPR dest, FPR op1, FPR op2, FPR op3) { // Paired single multiply-add scalar high
        write32 (0x1000003A | (dest << 21) | (op1 << 16) | (op3 << 11) | (op2 << 6) | setFlags);
    }

    template <bool setFlags = false>
    void ps_madds0 (FPR dest, FPR op1, FPR op2, FPR op3) { // Paired single multiply-add scalar high
        write32 (0x1000001C | (dest << 21) | (op1 << 16) | (op3 << 11) | (op2 << 6) | setFlags);
    }

    template <bool setFlags = false>
    void ps_madds1 (FPR dest, FPR op1, FPR op2, FPR op3) { // Paired single multiply-add scalar low
        write32 (0x1000001E | (dest << 21) | (op1 << 16) | (op3 << 11) | (op2 << 6) | setFlags);
    }

    template <bool setFlags = false>
    void ps_merge00 (FPR dest, FPR src1, FPR src2) { // Paired single merge high
        write32 (0x10000420 | (dest << 21) | (src1 << 16) | (src2 << 11) | setFlags);
    }

    template <bool setFlags = false>
    void ps_merge01 (FPR dest, FPR src1, FPR src2) { // Paired single merge direct
        write32 (0x10000460 | (dest << 21) | (src1 << 16) | (src2 << 11) | setFlags);
    }

    template <bool setFlags = false>
    void ps_merge10 (FPR dest, FPR src1, FPR src2) { // Paired single merge swapped
        write32 (0x100004A0 | (dest << 21) | (src1 << 16) | (src2 << 11) | setFlags);
    }

    template <bool setFlags = false>
    void ps_merge11 (FPR dest, FPR src1, FPR src2) { // Paired single merge low
        write32 (0x100004E0 | (dest << 21) | (src1 << 16) | (src2 << 11) | setFlags);
    }

    template <bool setFlags = false>
    void ps_mr (FPR dest, FPR src) { // Paired snigle move register
        write32 (0x10000090 | (dest << 21) | (src << 11) | setFlags);
    }

    template <bool setFlags = false>
    void ps_msub (FPR dest, FPR op1, FPR op2, FPR op3) { // Paired single multiply-subtract
        write32 (0x10000038 | (dest << 21) | (op1 << 16) | (op3 << 11) | (op2 << 6) | setFlags);
    }

    template <bool setFlags = false>
    void ps_mul (FPR dest, FPR src1, FPR src2) { // Paired single add
        write32 (0x10000032 | (dest << 21) | (src1 << 16) | (src2 << 6) | setFlags);
    }

    template <bool setFlags = false>
    void ps_muls0 (FPR dest, FPR src1, FPR src2) { // Paired single multiply scalar high
        write32 (0x10000018 | (dest << 21) | (src1 << 16) | (src2 << 6) | setFlags);
    }

    template <bool setFlags = false>
    void ps_muls1 (FPR dest, FPR src1, FPR src2) { // Paired single multiply scalar high
        write32 (0x1000001A | (dest << 21) | (src1 << 16) | (src2 << 6) | setFlags);
    }

    template <bool setFlags = false>
    void ps_nabs (FPR dest, FPR src) { // Paired single negative absolute value
        write32 (0x10000110 | (dest << 21) | (src << 11) | setFlags);
    }

    template <bool setFlags = false>
    void ps_neg (FPR dest, FPR src) { // Paired single negative
        write32 (0x10000050 | (dest << 21) | (src << 11) | setFlags);
    }

    template <bool setFlags = false>
    void ps_nmadd (FPR dest, FPR op1, FPR op2, FPR op3) { // Paired single negative multiply add
        write32 (0x1000003E | (dest << 21) | (op1 << 16) | (op3 << 11) | (op2 << 6) | setFlags);
    }

    template <bool setFlags = false>
    void ps_nmsub (FPR dest, FPR op1, FPR op2, FPR op3) { // Paired single negative multiply add
        write32 (0x1000003C | (dest << 21) | (op1 << 16) | (op3 << 11) | (op2 << 6) | setFlags);
    }
    
    template <bool setFlags = false>
    void ps_res (FPR dest, FPR src) { // Paired single reciprocal estimate
        write32 (0x10000030 | (dest << 21) | (src << 11) | setFlags);
    }

    template <bool setFlags = false>
    void ps_rsqrte (FPR dest, FPR src) { // Paired single reciprocal square root estimate
        write32 (0x10000034 | (dest << 21) | (src << 11) | setFlags);
    }

    template <bool setFlags = false>
    void ps_sel (FPR dest, FPR op1, FPR op2, FPR op3) { // Paired single select
        write32 (0x1000002E | (dest << 21) | (op1 << 16) | (op3 << 11) | (op2 << 6) | setFlags);
    }

    template <bool setFlags = false>
    void ps_sub (FPR dest, FPR src1, FPR src2) { // Paired single subtract
        write32 (0x10000028 | (dest << 21) | (src1 << 16) | (src2 << 11) | setFlags);
    }

    template <bool setFlags = false>
    void ps_sum0 (FPR dest, FPR op1, FPR op2, FPR op3) { // Paired single vector sum high
        write32 (0x10000014 | (dest << 21) | (op1 << 16) | (op3 << 11) | (op2 << 6) | setFlags);
    }

    template <bool setFlags = false>
    void ps_sum1 (FPR dest, FPR op1, FPR op2, FPR op3) { // Paired single vector sum low
        write32 (0x10000016 | (dest << 21) | (op1 << 16) | (op3 << 11) | (op2 << 6) | setFlags);
    }

    // AltiVec SIMD ISA
    void dss (uint8_t stream) { // Data Stream Stop
        write32 (0x7C00066C | (stream << 21));
    }

    void dssall() { write32 (0x7E00066C); } // Data Stream Stop All

    void vaddfp (VR dest, VR src1, VR src2) { // Vector Add Floating-Point (32-bit)
		write32 (0x1000000A | (dest << 21) | (src1 << 16) | (src2 << 11));
	}

    void vsubfp (VR dest, VR src1, VR src2) { // Vector sub Floating-Point (32-bit)
        write32 (0x1000004A | (dest << 21) | (src1 << 16) | (src2 << 11));
    }
};
} // End Namespace Luma
