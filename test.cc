// This is a program made to check the emitter for regressions by the CI
// Horrible code will ensue
#define RUNNING_IN_CI 1
#include <iostream>
#include <iterator>
#include <vector>
#include <string>
#include "luma.hpp"
using namespace Luma;

// Example of making a derived class to add custom extensions
namespace Luma { // Extend Emitter namespace to add new types
enum FancyNewRegisterType { // Defining an enum for a hypothetical new register type
    MyReg0, MyReg1, MyReg2, MyReg3
};

class ExtendedEmitter : public PPCEmitter <FixedSize> { // Making an emitter extension to support the extra stuff we want
public:
    using PPCEmitter::PPCEmitter; // Use PPCEmitter constructors
    
    void MyInstruction (FancyNewRegisterType dest, FancyNewRegisterType src) { // Adding support for a new instruction using our new register type
        dw (0x6000003A | (dest << 21) | (src << 16)); // Emit a 32-bit instruction in this format
    }
};
} // end Namespace Luma

static std::vector <uint8_t> loadBinary(std::string directory) {
    std::ifstream file (directory, std::ios::binary);
    std::vector <uint8_t> vec;

    file.unsetf(std::ios::skipws);
    vec.insert(vec.begin(),
                std::istream_iterator<uint8_t>(file),
                std::istream_iterator<uint8_t>());

    file.close();
    return vec;
}

int main() {
    ExtendedEmitter gen;

    const auto label1 = gen.beq();
    gen.mflr (r3);
    gen.stw (r3, sp, 0);
    const auto a = gen.getCurr();
    gen.nop();
    gen.setLabel (label1);

    const auto label2 = gen.bne(); 
    gen.lwzu (r0, r1, -4);
    gen.lhz (r2, r1, -16);
    gen.setLabel (label2, a);

    gen.lhzu (r1, r2, -69);
    gen.lbzu (r0, r31, 0);
    gen.lbz (r0, r1, 1);
    gen.lbzux (r0, r1, r2);
    gen.lbzx (r10, r12, r4);
    gen.lhzux (r0, r3, r2);
    gen.lhzx (r6, r7, r9);
    gen.lwzx (r31, r30, r29);
    gen.lwzux (r2, r30, r31);

    gen.lmw (r31, r15, -120);
    gen.stmw (r29, r30, -4040);
    gen.stwux (r0, r10, r3);
    gen.stwx (r9, r12, r3);

    gen.stb (r1, r2, 4);
    gen.sth (r1, r2, 12);
    gen.stbu (r1, r2, -4);
    gen.sthu (r2, r3, -8);

    gen.stfd (f0, r4, -8);
    gen.lfd (f19, r8, -90);
    gen.vaddfp (v1, v2, v0);
    gen.fmr <false> (f0, f31);
    gen.fmr <true> (f0, f31);
    gen.fadd <false> (f2, f3, f0);
    gen.fadd <true> (f2, f3, f0);
    gen.fadds (f2, f3, f0);
    gen.fdiv (f0, f0, f0);
    gen.fdivs (f0, f0, f0);
    gen.fmadd <false> (f0, f4, f1, f3);
    gen.fmadd <true> (f0, f4, f1, f3);
    gen.fmadds (f1, f19, f0, f30);
    gen.fmsub (f0, f9, f10, f20);
    gen.fmsubs <false> (f1, f9, f10, f20);
    gen.fmsubs <true> (f1, f9, f10, f20);
    gen.fnabs <false> (f0, f4);
    gen.fnabs <true> (f0, f4);
    gen.fmul <false> (f1, f3, f9);
    gen.fmul <true> (f1, f3, f9);
    gen.fneg <false> (f0, f2);
    gen.fneg <true> (f0, f2);
    gen.fnmadd <false> (f1, f10, f20, f30);
    gen.fnmadd <true> (f1, f10, f20, f30);
    gen.fnmadds (f30, f20, f10, f0);

    gen.fnmsub <false> (f1, f10, f20, f30);
    gen.fnmsub <true> (f1, f10, f20, f30);
    gen.fnmsubs <false> (f21, f11, f1, f31);
    gen.fnmsubs <true> (f21, f11, f1, f31);
    gen.frsqrte <false> (f0, f10);
    gen.frsqrte <true> (f0, f10);
    gen.frsp (f1, f2);
    gen.fres (f10, f20);
    gen.fsel <false> (f1, f0, f10, f20);
    gen.fsel <true> (f1, f0, f10, f20);

    gen.fsub <false> (f0, f12, f21);
    gen.fsub <true> (f0, f12, f21);
    gen.fsubs (f1, f1, f3);
    gen.cmpi (cr1, r1, -69);
    gen.cmpl (cr7, r7, r9);
    gen.cmpli (cr2, r9, 23);
    gen.cntlzw (r0, r1);

    gen.icbi (r1, r31);
    gen.dcbf (r9, r13);
    gen.dcbst (r12, r3);
    gen.dcbi (r1, r2);
    gen.dcbt (r9, r20);
    gen.dcbtst (r5, r4);
    gen.dcbz (r2, r1);
    gen.dcbz_l (r13, r16);

    gen.subf (r1, r3, r4);
    gen.subfo <false> (r0, r9, r27);
    gen.subfo <true> (r0, r9, r27);
    gen.addo <false> (r0, r17, r16);
    gen.addo <true> (r0, r17, r16);
    gen.addc (r15, r21, r7);
    gen.addco <false> (r1, r3, r5);
    gen.addco <true> (r1, r3, r5);
    gen.subfc <false> (r19, r23, r24);
    gen.subfc <true> (r19, r23, r24);
    gen.subfco (r1, r2, r4);
    gen.addeo <false> (r0, r13, r9);
    gen.addeo <true> (r0, r13, r9);
    gen.adde (r12, r4, r3);

    gen.addic (r0, r4, -4);
    gen.addi (r1, r9, 24);
    gen.addic <false> (r1, r4, -40);
    gen.addic <true> (r1, r4, -40);
    gen.addis (r0, r2, -1);

    gen.addmeo <false> (r9, r10);
    gen.addmeo <true> (r9, r10);
    gen.addme (r0, r11);
    gen.subfic (r1, r2, -8);

    gen.subfme (r1, r9);
    gen.subfmeo <false> (r1, r0);
    gen.subfmeo <true> (r1, r0);
    gen.subfzeo (r9, r31);
    gen.subfze <false> (r2, r1);
    gen.subfze <true> (r2, r1);

    gen.addze (r1, r2);
    gen.addzeo <false> (r0, r9);
    gen.addzeo <true> (r0, r9);
    gen.eieio();
    gen.isync();
    gen.sync();

    gen.divw <false> (r1, r9, r10);
    gen.divwo <true> (sp, r2, r3);
    gen.mulli (r0, r3, -9);
    gen.mullw <false> (r3, r4, r21);
    gen.mullw <true> (r3, r4, r21);
    gen.mullwo <false> (r3, r4, r21);
    gen.mullwo <true> (r3, r4, r21);
    gen.mulhw <false> (r9, r12, r14);
    gen.mulhw <true> (r9, r12, r14);
    gen.mulhwu <false> (r1, r3, r5); 
    gen.mulhwu <true> (r1, r3, r5);

    gen.divwu <false> (r0, r9, r13);
    gen.divwu <true> (r0, r9, r13);
    gen.divwuo <false> (r13, sp, r15);
    gen.divwuo <true> (r13, sp, r15);

    gen.lhbrx (r1, r3, r4);
    gen.lhax (r2, r4, r6);
    gen.lhaux (r9, r13, r15);
    gen.lwbrx (r9, r1, r12);
    gen.lwarx (r12, r14, r16);

    gen.mtcrf (0xFF, sp); // Equivalent to mtcr r1
    gen.mtsr (sr9, r10);
    gen.mfsr (r3, sr7);
    gen.mtsrin (r9, r10);
    gen.mfsrin (r12, r15);
    gen.mfmsr (r9);
    gen.mtmsr (r30);

    gen.mtlr (r29);
    gen.mflr (r20);
    gen.mtctr (r30);
    gen.mfctr (r1);

    gen.and_ <false> (r1, r4, r9);
    gen.and_ <true> (r1, r4, r9);
    gen.or_ <false> (r7, r10, r2);
    gen.or_ <true> (r7, r10, r2);
    gen.xor_ <false> (r1, r12, r23);
    gen.xor_ <true> (r1, r12, r23);
    
    gen.ps_abs (f9, f23);
    gen.ps_abs <true> (f9, f23);
    gen.ps_add (f21, f26, f28);
    gen.ps_add <true> (f21, f26, f28);
    gen.ps_cmpo0 (cr6, f0, f1);
    gen.ps_cmpo1 (cr3, f4, f5);
    gen.ps_cmpu0 (cr1, f30, f31);
    gen.ps_cmpu1 (cr2, f24, f25);
    gen.ps_div (f1, f0, f3);
    gen.ps_div <true> (f1, f0, f3);
    gen.ps_madds0 (f3, f9, f4, f5);
    gen.ps_madds0 <true> (f3, f9, f4, f5);
    gen.ps_madds1 (f3, f9, f4, f5);
    gen.ps_madds1 <true> (f3, f9, f4, f5);
    gen.ps_merge00 (f3, f4, f0);
    gen.ps_merge00 <true> (f3, f4, f0);
    gen.ps_merge01 (f3, f4, f0);
    gen.ps_merge01 <true> (f3, f4, f0);
    gen.ps_merge10 (f3, f4, f0);
    gen.ps_merge10 <true> (f3, f4, f0);
    gen.ps_merge11 (f3, f4, f0);
    gen.ps_merge11 <true> (f3, f4, f0);
    
    gen.ps_msub (f3, f4, f5, f6);
    gen.ps_msub <true> (f3, f4, f5, f6);
    gen.ps_mul (f4, f9, f10);
    gen.ps_mul <true> (f4, f9, f10);
    gen.ps_muls0 (f4, f9, f10);
    gen.ps_muls0 <true> (f4, f9, f10);
    gen.ps_muls1 (f4, f9, f10);
    gen.ps_muls1 <true> (f4, f9, f10);
    gen.ps_nabs (f15, f19);
    gen.ps_nabs <true> (f15, f19);
    gen.ps_neg (f15, f19);
    gen.ps_neg <true> (f15, f19);
    gen.ps_rsqrte (f0, f3);
    gen.ps_rsqrte <true> (f0, f3);

    gen.ps_sel (f0, f3, f4, f9);
    gen.ps_sel <true> (f0, f3, f4, f9);
    gen.ps_sum0 (f3, f4, f9, f10);
    gen.ps_sum0 <true> (f3, f4, f9, f10);
    gen.ps_sum1 (f1, f2, f3, f4);
    gen.ps_sum1 <true> (f1, f2, f3, f4);

    gen.rlwinm (r3, r4, 20, 0, 16);
    gen.rlwimi (r23, r6, 12, 10, 20);
    gen.rotlwi (r1, r2, 5);
    gen.rotrwi (r9, r20, 10);
    gen.rlwnm (r9, r2, r4, 0, 31);
    gen.rfi();
    gen.slw (r9, r10, r11);
    gen.srw (r9, r10, r11);
    gen.sraw (r9, r10, r11);
    gen.srawi (r9, r10, 10);
    gen.tlbsync();
    gen.tlbie (r12);
    gen.rfi();
    gen.extrwi (r4, r10, 5, 10);
    gen.extlwi (r3, r9, 11, 17);
    gen.oris (r3, r5, 10);
    gen.ori (r2, r1, 0xFFFF);

    const auto l1 = gen.ble();
    const auto l2 = gen.bgt();
    const auto l3 = gen.blt();
    const auto l4 = gen.bge();
    const auto l5 = gen.bne();
    const auto l6 = gen.beq();
    const auto l7 = gen.bso();
    const auto l8 = gen.bns();
    const auto l9 = gen.blel();
    const auto l10 = gen.bgtl();
    const auto l11 = gen.bltl();
    const auto l12 = gen.bgel();
    const auto l13 = gen.bnel();
    const auto l14 = gen.beql();
    const auto l15 = gen.bsol();
    const auto l16 = gen.bnsl();

    gen.setLabel (l1);
    gen.setLabel (l2);
    gen.setLabel (l3);
    gen.setLabel (l4);
    gen.setLabel (l5);
    gen.setLabel (l6);
    gen.setLabel (l7);
    gen.setLabel (l8);
    gen.setLabel (l9);
    gen.setLabel (l10);
    gen.setLabel (l11);
    gen.setLabel (l12);
    gen.setLabel (l13);
    gen.setLabel (l14);
    gen.setLabel (l15);
    gen.setLabel (l16);
    gen.ud(); // undefined opcode

    gen.df64 (69.420); // test emitting double data
    uint16_t testArray[] = { 4, 10, 0xFFFF }; // test emitting an array of words
    gen.dh (testArray, 3);
    gen.align (4); // test aligning to a word boundary
    gen.andis (r25, r28, 123);
    gen.dss (2);
    gen.dssall();
    gen.li (r9, -10);
    gen.li (r8, 10);
    gen.liu (r9, 0xFFFE);
    gen.liu (r7, 10);
    gen.lis (r30, 10);
    gen.lis (r9, 0xF000);
    gen.ps_sel <true> (f0, f1, f9, f3);

    gen.liw (r10, 0x8000);
    gen.liw (r12, 0x999);
    gen.liw (r1, 0xFFFFF000);
    gen.liw (r31, 0x12345678);

    const auto label9 = gen.bl();
    gen.setLabel (label9);
    gen.vsubfp (v0, v9, v31);
    gen.clrlwi (r1, r2, 10);
    gen.clrrwi (r9, r30, 5);
    gen.clrlwi <true> (r27, r20, 19);
    gen.setz (r0, r20);
    gen.mfcr (r9);
    gen.mfcr (r3);

    gen.repeat <10> ([&](auto i) { // test repeat directive (should emit 10 NOPs and 10 addis with incrementing immediates)
        gen.nop();
        gen.addi (r0, r1, i);
    });

    gen.loop (r3, 69, [&]() { // test loop directive (should emit a 69 iteration loop of nop and isync, using r3 as counter)
        gen.nop();
        gen.isync();
    });

    gen.ds ("*boop* *boop* *boop*"); // test C-strings
    gen.ds (std::string("*boop* *boop* *boop*")); // test std::strings

    gen.align (4);
    gen.vnor (v9, v3, v4);
    gen.vor (v10, v31, v20);
    gen.vxor (v1, v2, v3);
    gen.vand (v30, v13, v12);
    gen.vandc (v15, v12, v0);
    gen.vperm (v1, v10, v20, v30);
    gen.vrefp (v17, v23);

    // Time to check the code for regressions
    if (RUNNING_IN_CI) { // Check if this is running in CI
        const auto correctFile = loadBinary (".github/test_binaries/binary1.bin");
        if (correctFile.empty()) { // Error if the file wasn't found
            printf ("Test failure. Failed to find file for correct binary\n");
            return -1;
        }

        const auto blocks = (uint8_t*) gen.getBuffer();
        for (auto i = 0; i < gen.getCodeSize(); i++) {
            if (blocks[i] != correctFile[i]) {
                printf ("Test failure. Created binary does not match with correct binary. Failed at byte: %d\n", i);
                printf ("Expected: %02X     Got: %02X\n", correctFile[i], blocks[i]);
                return -1;
            }
        }

        printf ("Test passed successfully\n");
    }

    else // If we're not running in CI, dump the file
        gen.dump (".github/test_binaries/binary1.bin");

    return 0;
}
