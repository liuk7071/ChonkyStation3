#pragma once

#include <BitField.hpp>


namespace PPUTypes {

struct ConditionRegister {
    u32 raw = 0;
    enum {
        LESS    = 0b1000,
        GREATER = 0b0100,
        EQUAL   = 0b0010
    };
    
    void setCRField(u8 n, u8 v, u8 so = 0) {
        const auto bit = 28 - (n * 4);  // CR Fields are reversed (CR0 is bits 28-31)
        raw &= ~(0b1111 << bit);
        raw |= (v | so) << bit;
    }

    u8 getCRField(u8 n) {
        const auto bit = 28 - (n * 4);  // CR Fields are reversed (CR0 is bits 28-31)
        return (raw >> bit) & 0b1111;
    }

    template <typename T>
    void compareAndUpdateCRField(u8 n, T a, T b, u8 so = 0) {
        if (a < b) setCRField(n, LESS, so);
        else if (a > b) setCRField(n, GREATER, so);
        else setCRField(n, EQUAL, so);
    }
};

struct XER {
    u8 n_bytes = 0;
    bool ca = false;
    bool ov = false;
    bool so = false;
    u64 get() {
        return (so << 31) | (ov << 30) | (ca << 29) | (n_bytes & 0x7f);
    }
};

using VR = v128;

struct State {
    u64 gprs[32] = { 0 };
    double fprs[32] = { 0.0 };
    VR vrs[32] = { 0 };

    u64 pc = 0;
    u64 lr = 0;
    ConditionRegister cr;
    XER xer;
    u64 ctr = 0;
    u32 vrsave = -1;

    PPUTypes::State& operator=(const PPUTypes::State& other) {
        for (int i = 0; i < 32; i++) gprs[i] = other.gprs[i];
        for (int i = 0; i < 32; i++) fprs[i] = other.fprs[i];
        for (int i = 0; i < 32; i++) {
            vrs[i].dw[0] = other.vrs[i].dw[0];
            vrs[i].dw[1] = other.vrs[i].dw[1];
        }
        pc = other.pc;
        lr = other.lr;
        xer.n_bytes = other.xer.n_bytes;
        xer.ca = other.xer.ca;
        xer.ov = other.xer.ov;
        xer.so = other.xer.so;
        cr.raw = other.cr.raw;
        ctr = other.ctr;
        vrsave = other.vrsave;
        return *this;
    }
};

union Instruction {
    u32 raw;
    BitField<0,  1,  u32> lk;
    BitField<0,  1,  u32> rc;           // lk == rc
    BitField<0,  2,  u32> g_3a_field;
    BitField<0,  2,  u32> g_3e_field;   // g_3a_field == g_3e_field
    BitField<0,  11, u32> g_04_field;
    BitField<0,  16, u32> ui;
    BitField<0,  16, u32> d;            // ui == d
    BitField<0,  16, u32> si;           // ui == si
    BitField<1,  1,  u32> aa;
    BitField<1,  1,  u32> sh_hi;        // aa == sh_hi
    BitField<1,  5,  u32> me_5;
    BitField<1,  5,  u32> g_3b_field;   // me_5 == g_3b_field
    BitField<1,  10, u32> g_13_field;
    BitField<1,  10, u32> g_1f_field;   // g_13_field == g_1f_field
    BitField<1,  10, u32> g_3f_field;   // g_13_field == g_3f_field
    BitField<1,  4,  u32> g_1e_field;
    BitField<2,  14, u32> ds;
    BitField<2,  14, u32> bd;           // ds == bd
    BitField<2,  24, u32> li;
    BitField<6,  5,  u32> vc;
    BitField<5,  6,  u32> mb_6;
    BitField<5,  6,  u32> me_6;
    BitField<6,  4,  u32> shb;
    BitField<6,  5,  u32> mb_5;
    BitField<6,  5,  u32> frc;          // mb_5 == frc
    BitField<10, 1,  u32> oe;
    BitField<10, 1,  u32> rc_v;         // oe == rc_v   (rc bit in altivec instructions is bit 21 instead of 31)
    BitField<11, 3,  u32> bh;
    BitField<11, 5,  u32> rb;
    BitField<11, 5,  u32> frb;          // rb == frb
    BitField<11, 5,  u32> vb;           // rb == vb
    BitField<11, 5,  u32> sh_lo;        // rb == sh_lo
    BitField<11, 5,  u32> sh;           // rb == sh
    BitField<11, 5,  u32> bb;           // rb == bb
    BitField<11, 10, u32> spr;
    BitField<12, 8,  u32> fxm;
    BitField<16, 5,  u32> ra;
    BitField<16, 5,  u32> fra;          // ra == fra
    BitField<16, 5,  u32> va;           // ra == va
    BitField<16, 5,  u32> uimm;         // ra == uimm
    BitField<16, 5,  u32> simm;         // ra == simm
    BitField<16, 5,  u32> bi;           // ra == bi
    BitField<16, 5,  u32> ba;           // rt == ba
    BitField<18, 3,  u32> bfa;
    BitField<20, 1,  u32> one;
    BitField<21, 1,  u32> l;
    BitField<21, 5,  u32> rt;
    BitField<21, 5,  u32> frt;          // rt == frt
    BitField<21, 5,  u32> rs;           // rt == rs
    BitField<21, 5,  u32> frs;          // rt == frs
    BitField<21, 5,  u32> vd;           // rt == vd
    BitField<21, 5,  u32> vs;           // rt == vs
    BitField<21, 5,  u32> bo;           // rt == bo
    BitField<21, 5,  u32> bt;           // rt == bt
    BitField<23, 3,  u32> bf;
    BitField<26, 6,  u32> opc;
};

static const u64 lvsl_shifts[0x10][2] = {
    { 0x08090A0B0C0D0E0F, 0x0001020304050607 },
    { 0x090A0B0C0D0E0F10, 0x0102030405060708 },
    { 0x0A0B0C0D0E0F1011, 0x0203040506070809 },
    { 0x0B0C0D0E0F101112, 0x030405060708090A },
    { 0x0C0D0E0F10111213, 0x0405060708090A0B },
    { 0x0D0E0F1011121314, 0x05060708090A0B0C },
    { 0x0E0F101112131415, 0x060708090A0B0C0D },
    { 0x0F10111213141516, 0x0708090A0B0C0D0E },
    { 0x1011121314151617, 0x08090A0B0C0D0E0F },
    { 0x1112131415161718, 0x090A0B0C0D0E0F10 },
    { 0x1213141516171819, 0x0A0B0C0D0E0F1011 },
    { 0x131415161718191A, 0x0B0C0D0E0F101112 },
    { 0x1415161718191A1B, 0x0C0D0E0F10111213 },
    { 0x15161718191A1B1C, 0x0D0E0F1011121314 },
    { 0x161718191A1B1C1D, 0x0E0F101112131415 },
    { 0x1718191A1B1C1D1E, 0x0F10111213141516 }
};

static const u64 lvsr_shifts[0x10][2] = {
    {0x18191A1B1C1D1E1F, 0x1011121314151617},
    {0x1718191A1B1C1D1E, 0x0F10111213141516},
    {0x161718191A1B1C1D, 0x0E0F101112131415},
    {0x15161718191A1B1C, 0x0D0E0F1011121314},
    {0x1415161718191A1B, 0x0C0D0E0F10111213},
    {0x131415161718191A, 0x0B0C0D0E0F101112},
    {0x1213141516171819, 0x0A0B0C0D0E0F1011},
    {0x1112131415161718, 0x090A0B0C0D0E0F10},
    {0x1011121314151617, 0x08090A0B0C0D0E0F},
    {0x0F10111213141516, 0x0708090A0B0C0D0E},
    {0x0E0F101112131415, 0x060708090A0B0C0D},
    {0x0D0E0F1011121314, 0x05060708090A0B0C},
    {0x0C0D0E0F10111213, 0x0405060708090A0B},
    {0x0B0C0D0E0F101112, 0x030405060708090A},
    {0x0A0B0C0D0E0F1011, 0x0203040506070809},
    {0x090A0B0C0D0E0F10, 0x0102030405060708},
};

enum Instructions {
    TDI     = 0x02,     // Trap Doubleword Immediate 
    TWI     = 0x03,     // Trap Word Immediate
    G_04    = 0x04,
    MULLI   = 0x07,     // Multiply Low Immediate
    SUBFIC  = 0x08,     // Subtract from Immediate Carrying
    DOZI    = 0x09,     // (deleted instruction)
    CMPLI   = 0x0a,     // Compare Logical Immediate
    CMPI    = 0x0b,     // Compare Immediate
    ADDIC   = 0x0c,     // Add Immediate Carrying
    ADDIC_  = 0x0d,     // Add Immediate Carrying and Record
    ADDI    = 0x0e,     // Add Immediate
    ADDIS   = 0x0f,     // Add Immediate Shifted
    BC      = 0x10,     // Branch Conditional
    SC      = 0x11,     // System Call
    B       = 0x12,     // Branch
    G_13    = 0x13,
    RLWIMI  = 0x14,     // Rotate Left Word Immediate then Mask Insert
    RLWINM  = 0x15,     // Rotate Left Word Immediate then AND with Mask
    RLWNM   = 0x17,     // Rotate Left Word then AND with Mask
    ORI     = 0x18,     // OR Immediate
    ORIS    = 0x19,     // OR Immediate Shifted
    XORI    = 0x1a,     // XOR Immediate
    XORIS   = 0x1b,     // XOR Immediate Shifted
    ANDI    = 0x1c,     // AND Immediate
    ANDIS   = 0x1d,     // AND Immediate Shifted
    G_1E    = 0x1e,
    G_1F    = 0x1f,
    LWZ     = 0x20,     // Load Word and Zero
    LWZU    = 0x21,     // Load Word and Zero with Update
    LBZ     = 0x22,     // Load Byte and Zero
    LBZU    = 0x23,     // Load Byte and Zero with Update
    STW     = 0x24,     // Store Word
    STWU    = 0x25,     // Store Word with Update
    STB     = 0x26,     // Store Byte
    STBU    = 0x27,     // Store Byte with Update
    LHZ     = 0x28,     // Load Halfword and Zero
    LHZU    = 0x29,     // Load Halfword and Zero with Update
    LHA     = 0x2a,     // Load Halfword Algebraic with Update
    LHAU    = 0x2b,     // Load Halfword Algebraic
    STH     = 0x2c,     // Store Halfword
    STHU    = 0x2d,     // Store Halfword with Update
    LMW     = 0x2e,     // Load Multiple Word
    STMW    = 0x2f,     // Store Multiple Word
    LFS     = 0x30,     // Load Floating-Point Single
    LFSU    = 0x31,     // Load Floating-Point Single with Update
    LFD     = 0x32,     // Load Floating-Point Double
    LFDU    = 0x33,     // Load Floating-Point Double with Update
    STFS    = 0x34,     // Store Floating-Point Single
    STFSU   = 0x35,     // Store Floating-Point Single with Update
    STFD    = 0x36,     // Store Floating-Point Double
    STFDU   = 0x37,     // Store Floating-Point Double with Update
    LFQ     = 0x38,
    LFQU    = 0x39,
    G_3A    = 0x3a,
    G_3B    = 0x3b,
    G_3E    = 0x3e,
    G_3F    = 0x3f,
};

enum G_04Opcodes {
    VADDUBM         = 0x0, 
    VMAXUB          = 0x2, 
    VRLB            = 0x4, 
    VCMPEQUB        = 0x006,    // Vector Compare Equal Unsigned Byte
    VCMPEQUB_       = 0x406,
    VMULOUB         = 0x8, 
    VADDFP          = 0xa,      // Vector Add Floating-Point
    VMRGHB          = 0xc, 
    VPKUHUM         = 0xe, 
    VMHADDSHS       = 0x20,
    VMHRADDSHS      = 0x21,     // Vector Multiply High Round and Add Signed Half Word Saturate
    VMLADDUHM       = 0x22,     // Vector Multiply Low and Add Unsigned Half Word Modulo
    VMSUMUBM        = 0x24,
    VMSUMMBM        = 0x25,
    VMSUMUHM        = 0x26,
    VMSUMUHS        = 0x27,
    VMSUMSHM        = 0x28,     // Vector Multiply Sum Signed Half Word Modulo
    VMSUMSHS        = 0x29,
    VSEL            = 0x2a,     // Vector Conditional Select
    VPERM           = 0x2b,     // Vector Permute
    VSLDOI          = 0x2c,
    VMADDFP         = 0x2e,     // Vector Multiply Add Floating-Point
    VNMSUBFP        = 0x2f,     // Vector Negative Multiply-Subtract Floating-Point
    VADDUHM         = 0x40,     // Vector Add Unsigned Half Word Modulo
    VMAXUH          = 0x42,
    VRLH            = 0x44,
    VCMPEQUH        = 0x046,
    VCMPEQUH_       = 0x446,
    VMULOUH         = 0x48,     // Vector Multiply Odd Unsigned Half Word
    VSUBFP          = 0x4a,     // Vector Subtract Floating-Point
    VMRGHH          = 0x4c,     // Vector Merge High Half word
    VPKUWUM         = 0x4e,
    VADDUWM         = 0x80,     // Vector Add Unsigned Word Modulo
    VMAXUW          = 0x82,
    VRLW            = 0x84,     // Vector Rotate Left Integer Word
    VCMPEQUW        = 0x086,    // Vector Compare Equal Unsigned Word
    VCMPEQUW_       = 0x486,
    VMRGHW          = 0x8c,     // Vector Merge High Word
    VPKUHUS         = 0x8e,
    VCMPEQFP        = 0x0c6,    // Vector Compare Equal-to-Floating Point
    VCMPEQFP_       = 0x4c6,
    VPKUWUS         = 0xce,
    VMAXSB          = 0x102,
    VSLB            = 0x104,
    VMULOSB         = 0x108,
    VREFP           = 0x10a,    // Vector Reciprocal Estimate Floating-Point
    VMRGLB          = 0x10c,
    VPKSHUS         = 0x10e,    // Vector Pack Signed Half Word Unsigned Saturate
    VMAXSH          = 0x142,
    VSLH            = 0x144,    // Vector Shift Left Integer Half Word          
    VMULOSH         = 0x148,    // Vector Multiply Odd Signed Half Word
    VRSQRTEFP       = 0x14a,    // Vector Reciprocal Square Root Estimate Floating-Point
    VMRGLH          = 0x14c,    // Vector Merge Low Half Word
    VPKSWUS         = 0x14e,
    VADDCUW         = 0x180,
    VMAXSW          = 0x182,
    VSLW            = 0x184,    // Vector Shift Left Integer Word
    VEXPTEFP        = 0x18a,    // Vector 2 Raised to the Exponent Estimate Floating-Point
    VMRGLW          = 0x18c,    // Vector Merge Low Word
    VPKSHSS         = 0x18e,
    VSL             = 0x1c4,
    VCMPGEFP        = 0x1c6,    // Vector Compare Greater-Than-or-Equal-to Floating-Point
    VCMPGEFP_       = 0x5c6,
    VLOGEFP         = 0x1ca,
    VPKSWSS         = 0x1ce,    // Vector Pack Signed Word Signed Saturate
    VADDUBS         = 0x200,
    VMINUB          = 0x202,
    VSRB            = 0x204,
    VCMPGTUB        = 0x206,    // Vector Compare Greater-Than Unsigned Byte
    VCMPGTUB_       = 0x606,
    VMULEUB         = 0x208,
    VRFIN           = 0x20a,
    VSPLTB          = 0x20c,    // Vector Splat Byte
    VUPKHSB         = 0x20e,    // Vector Unpack High Signed Byte
    VADDUHS         = 0x240,
    VMINUH          = 0x242,
    VSRH            = 0x244,
    VCMPGTUH        = 0x246,    // Vector Compare Greater-Than Unsigned Half Word
    VCMPGTUH_       = 0x646,
    VMULEUH         = 0x248,
    VRFIZ           = 0x24a,
    VSPLTH          = 0x24c,    // Vector Splat Half Word
    VUPKHSH         = 0x24e,    // Vector Unpack High Signed Half Word
    VADDUWS         = 0x280,
    VMINUW          = 0x282,
    VSRW            = 0x284,    // Vector Shift Right Word
    VCMPGTUW        = 0x286,    // Vector Compare Greater-Than Unsigned Word
    VCMPGTUW_       = 0x686,
    VRFIP           = 0x28a,
    VSPLTW          = 0x28c,    // Vector Splat Word
    VUPKLSB         = 0x28e,    // Vector Unpack Low Signed Byte
    VSR             = 0x2c4,
    VCMPGTFP        = 0x2c6,    // Vector Compare Greater-Than Floating-Point
    VCMPGTFP_       = 0x6c6,
    VRFIM           = 0x2ca,
    VUPKLSH         = 0x2ce,    // Vector Unpack Low Signed Half Word
    VADDSBS         = 0x300,
    VMINSB          = 0x302,
    VSRAB           = 0x304,
    VCMPGTSB        = 0x306,
    VCMPGTSB_       = 0x706,
    VMULESB         = 0x308,
    VCFUX           = 0x30a,    // Vector Convert from Unsigned Fixed-Point Word
    VSPLTISB        = 0x30c,    // Vector Splat Immediate Signed Byte
    VPKPX           = 0x30e,
    VADDSHS         = 0x340,    // Vector Add Signed Half Word Saturate
    VMINSH          = 0x342,
    VSRAH           = 0x344,    // Vector Shift Right Algebraic Half Word
    VCMPGTSH        = 0x346,
    VCMPGTSH_       = 0x746,
    VMULESH         = 0x348,    // Vector Multiply Even Signed Half Word
    VCFSX           = 0x34a,    // Vector Convert from Signed Fixed-Point Word
    VSPLTISH        = 0x34c,    // Vector Splat Immediate Signed Half Word
    VUPKHPX         = 0x34e,
    VADDSWS         = 0x380,
    VMINSW          = 0x382,
    VSRAW           = 0x384,    // Vector Shift Right Algebraic Word
    VCMPGTSW        = 0x386,    // Vector Compare Greater-Than Signed Word
    VCMPGTSW_       = 0x786,
    VCTUXS          = 0x38a,    // Vector Convert to Unsigned Fixed-Point Word Saturate
    VSPLTISW        = 0x38c,    // Vector Splat Immediate Signed Word
    VCMPBFP         = 0x3c6,
    VCMPBFP_        = 0x7c6,
    VCTSXS          = 0x3ca,    // Vector Convert to Signed Fixed-Point Word Saturate
    VUPKLPX         = 0x3ce,
    VSUBUBM         = 0x400,
    VAVGUB          = 0x402,
    VAND            = 0x404,    // Vector AND
    VMAXFP          = 0x40a,    // Vector Maximum Floating-Point
    VSLO            = 0x40c,
    VSUBUHM         = 0x440,    // Vector Subtract Half Word Modulo
    VAVGUH          = 0x442,
    VANDC           = 0x444,    // Vector Logical AND with Complement
    VMINFP          = 0x44a,    // Vector Minimum Floating-Point
    VSRO            = 0x44c,
    VSUBUWM         = 0x480,    // Vector Subtract Word Modulo
    VAVGUW          = 0x482,
    VOR             = 0x484,    // Vector OR
    VXOR            = 0x4c4,    // Vector XOR
    VAVGSB          = 0x502,
    VNOR            = 0x504,    // Vector NOR
    VAVGSH          = 0x542,
    VSUBCUW         = 0x580,
    VAVGSW          = 0x582,
    VSUBUBS         = 0x600,
    MFVSCR          = 0x604,    // Move from Vector Status and Control Register
    VSUM4UBS        = 0x608,
    VSUBUHS         = 0x640,
    MTVSCR          = 0x644,    // Move to Vector Status and Control Register
    VSUM4SHS        = 0x648,
    VSUBUWS         = 0x680,
    VSUM2SWS        = 0x688,
    VSUBSBS         = 0x700,
    VSUM4SBS        = 0x708,    // Vector Sum Across Partial (1/4) Signed Byte Saturate
    VSUBSHS         = 0x740,    // Vector Subtract Signed Half Word Saturate
    VSUBSWS         = 0x780,
    VSUMSWS         = 0x788,    // Vector Sum Across Signed Word Saturate
};

enum G_13Opcodes {      // Field 21 - 30 
    MCRF    = 0x000,
    BCLR    = 0x010,
    CRNOR   = 0x021,
    CRANDC  = 0x081,
    ISYNC   = 0x096,
    CRXOR   = 0x0c1,
    CRNAND  = 0x0e1,
    CRAND   = 0x101,
    CREQV   = 0x121,
    CRORC   = 0x1a1,
    CROR    = 0x1c1,
    BCCTR   = 0x210,
};

enum G_1EOpcodes {      // Field 27 - 30
    RLDICL  = 0x00,
    RLDICL_ = 0x01,
    RLDICR  = 0x02,
    RLDICR_ = 0x03,
    RLDIC   = 0x04,
    RLDIC_  = 0x05,
    RLDIMI  = 0x06,
    RLDIMI_ = 0x07,
    RLDCL   = 0x08,
    RLDCR   = 0x09,
};

enum G_1FOpcodes {      // Field 21 - 30
    CMP     = 0x000,
    TW      = 0x004,
    LVSL    = 0x006,
    LVEBX   = 0x007,    // Load Vector Element Byte Indexed
    SUBFC   = 0x008,    // Subtract from Carrying
    MULHDU  = 0x009,
    ADDC    = 0x00a,
    MULHWU  = 0x00b,
    MFCR    = 0x013,
    LWARX   = 0x014,
    LDX     = 0x015,
    LWZX    = 0x017,
    SLW     = 0x018,
    CNTLZW  = 0x01a,
    SLD     = 0x01b,    // Shift Left Doubleword
    AND     = 0x01c,
    CMPL    = 0x020,    // Compare Logical
    LVSR    = 0x026,
    LVEHX   = 0x027,    // Load Vector Element Halfword Indexed
    SUBF    = 0x028,    // Subtract from
    LDUX    = 0x035,    // Load Doubleword with Update Indexed
    DCBST   = 0x036,
    LWZUX   = 0x037,
    CNTLZD  = 0x03a,
    ANDC    = 0x03c,
    LVEWX   = 0x047,    // Load Vector Element Word Indexed
    MULHD   = 0x049,
    MULHW   = 0x04b,
    LDARX   = 0x054,
    DCBF    = 0x056,
    LBZX    = 0x057,
    LVX     = 0x067,
    NEG     = 0x068,
    LBZUX   = 0x077,
    NOR     = 0x07c,
    STVEBX  = 0x087,
    SUBFE   = 0x088,    // Subtract from Extended
    ADDE    = 0x08a,
    MTCRF   = 0x090,
    STDX    = 0x095,
    STWCX_  = 0x096,
    STWX    = 0x097,
    STVEHX  = 0x0a7,
    STDUX   = 0x0b5,
    STVEWX  = 0x0c7,
    ADDZE   = 0x0ca,
    STDCX_  = 0x0d6,
    STBX    = 0x0d7,
    STVX    = 0x0e7,
    MULLD   = 0x0e9,
    ADDME   = 0x0ea,
    MULLW   = 0x0eb,
    DCBTST  = 0x0f6,
    STBUX   = 0x0f7,
    DOZ     = 0x108,
    ADD     = 0x10a,
    DCBT    = 0x116,
    LHZX    = 0x117,
    EQV     = 0x11c,
    ECIWX   = 0x136,
    LHZUX   = 0x137,
    XOR     = 0x13c,
    MFSPR   = 0x153,
    DST     = 0x156,
    LHAX    = 0x157,
    ABS     = 0x168,
    MFTB    = 0x173,
    DSTST   = 0x176,
    LHAUX   = 0x177,
    STHX    = 0x197,    // Store Halfword Indexed
    ORC     = 0x19c,    // OR with Complement
    ECOWX   = 0x1b6,
    OR      = 0x1bc,
    DIVDU   = 0x1c9,
    DIVWU   = 0x1cb,
    MTSPR   = 0x1d3,
    DCBI    = 0x1d6,
    NAND    = 0x1dc,
    DIVD    = 0x1e9,
    DIVW    = 0x1eb,
    LVLX    = 0x207,
    LWBRX   = 0x216,
    LFSX    = 0x217,
    SRW     = 0x218,
    SRD     = 0x21b,
    LVRX    = 0x227,
    LFSUX   = 0x237,
    SYNC    = 0x256,
    LFDX    = 0x257,
    LFDUX   = 0x277,
    STVLX   = 0x287,
    STFSX   = 0x297,
    STVRX   = 0x2a7,
    STFDX   = 0x2d7,
    LHBRX   = 0x316,
    SRAW    = 0x318,
    SRAD    = 0x31a,
    DSS     = 0x336,
    SRAWI   = 0x338,
    SRADI1  = 0x33a, 
    SRADI2  = 0x33b, 
    EIEIO   = 0x356,
    EXTSH   = 0x39a,    // Extend Sign Halfword
    EXTSB   = 0x3ba,
    STFIWX  = 0x3d7,
    EXTSW   = 0x3da,
    ICBI    = 0x3d6,
    DCBZ    = 0x3f6,
};

enum G_3AOpcodes {      // Field 30 - 31
    LD  = 0x00,
    LDU = 0x01,
    LWA = 0x02,
};

enum G_3BOpcodes {      // Field 26 - 30
    FDIVS   = 0x12,
    FSUBS   = 0x14,
    FADDS   = 0x15,
    FSQRTS  = 0x16,
    FRES    = 0x18,
    FMULS   = 0x19,
    FMSUBS  = 0x1c,
    FMADDS  = 0x1d,
    FNMSUBS = 0x1e,
    FNMADDS = 0x1f,
};

enum G_3EOpcodes {      // Field 30 - 31
    STD     = 0x00,     // Store Doubleword
    STDU    = 0x01,     // Store Doubleword with Update
};

enum G_3FOpcodes {      // Field 21 - 30
    MTFSB1  = 0x026,
    MCRFS   = 0x040,
    MTFSB0  = 0x046,
    MTFSFI  = 0x086,
    MFFS    = 0x247,
    MTFSF   = 0x2c7,
    FCMPU   = 0x000,    // Floating Compare Unordered
    FRSP    = 0x00c,    // Floating Round to Single-Precision
    FCTIW   = 0x00e,
    FCTIWZ  = 0x00f,    // Floating Convert To Integer Word with round toward Zero
    FDIV    = 0x012,
    FSUB    = 0x014,
    FADD    = 0x015,
    FSQRT   = 0x016,
    FSEL    = 0x017,
    FMUL    = 0x019,
    FRSQRTE = 0x01a,
    FMSUB   = 0x01c,
    FMADD   = 0x01d,
    FNMSUB  = 0x01e,
    FNMADD  = 0x01f,
    FCMPO   = 0x020,
    FNEG    = 0x028,
    FMR     = 0x048,
    FNABS   = 0x088,
    FABS    = 0x108,
    FCTID   = 0x32e,
    FCTIDZ  = 0x32f,
    FCFID   = 0x34e,    // Floating Convert From Integer Doubleword
};

} // End namespace PPUTypes
