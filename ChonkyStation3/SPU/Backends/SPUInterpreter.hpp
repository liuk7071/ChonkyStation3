#pragma once

#include <SPU.hpp>

#include <functional>
#include <cmath>
#if _MSC_VER
#include <intrin.h>
#pragma intrinsic(_mul128)
#endif


// Circular dependency
class PlayStation3;

using namespace SPUTypes;

static constexpr u32 INSTR_BITS = 11;
static constexpr u32 INSTR_TABLE_SIZE = 1 << INSTR_BITS;
static constexpr u32 INSTR_MASK = INSTR_TABLE_SIZE - 1;

class SPUInterpreter : public SPU {
public:
    SPUInterpreter(PlayStation3* ps3);
    int step() override;
    bool should_break = false;

    template<typename T, int l>
    T ext(T v) {
        return (T)(v << ((sizeof(T) * 8) - l)) >> ((sizeof(T) * 8) - l);
    }

    template<typename T>
    inline bool isShOK(int v) {
        return !(v >= sizeof(T) * 8);
    }

    template<typename T>
    inline T safeShl(T v, int sh) {
        return isShOK<T>(sh) ? (v << sh) : 0;
    }

    template<typename T>
    inline T safeShr(T v, int sh) {
        return isShOK<T>(sh) ? (v >> sh) : 0;
    }

    // Instruction table
    void (SPUInterpreter::*instr_table[INSTR_TABLE_SIZE])(const SPUInstruction&);
    void registerInstruction(u32 size, u32 opc, void (SPUInterpreter::*handler)(const SPUInstruction&));

    // Instructions
    void unimpl(const SPUInstruction& instr);

    void stop(const SPUInstruction& instr);
    void lnop(const SPUInstruction& instr);
    void sync(const SPUInstruction& instr);
    void dsync(const SPUInstruction& instr);
    void mfspr(const SPUInstruction& instr);
    void rdch(const SPUInstruction& instr);
    void rchcnt(const SPUInstruction& instr);
    void sf(const SPUInstruction& instr);
    void or_(const SPUInstruction& instr);
    void bg(const SPUInstruction& instr);
    void sfh(const SPUInstruction& instr);
    void nor(const SPUInstruction& instr);
    void absdb(const SPUInstruction& instr);
    void rot(const SPUInstruction& instr);
    void rotm(const SPUInstruction& instr);
    void rotma(const SPUInstruction& instr);
    void shl(const SPUInstruction& instr);
    void roth(const SPUInstruction& instr);
    void rothm(const SPUInstruction& instr);
    void rotmah(const SPUInstruction& instr);
    void shlh(const SPUInstruction& instr);
    void roti(const SPUInstruction& instr);
    void rotmi(const SPUInstruction& instr);
    void rotmai(const SPUInstruction& instr);
    void shli(const SPUInstruction& instr);
    void rothi(const SPUInstruction& instr);
    void rothmi(const SPUInstruction& instr);
    void rotmahi(const SPUInstruction& instr);
    void shlhi(const SPUInstruction& instr);
    void a(const SPUInstruction& instr);
    void and_(const SPUInstruction& instr);
    void cg(const SPUInstruction& instr);
    void ah(const SPUInstruction& instr);
    void nand(const SPUInstruction& instr);
    void avgb(const SPUInstruction& instr);
    void mtspr(const SPUInstruction& instr);
    void wrch(const SPUInstruction& instr);
    void biz(const SPUInstruction& instr);
    void binz(const SPUInstruction& instr);
    void bihz(const SPUInstruction& instr);
    void bihnz(const SPUInstruction& instr);
    void stopd(const SPUInstruction& instr);
    void stqx(const SPUInstruction& instr);
    void bi(const SPUInstruction& instr);
    void bisl(const SPUInstruction& instr);
    void iret(const SPUInstruction& instr);
    void bisled(const SPUInstruction& instr);
    void hbr(const SPUInstruction& instr);
    void gb(const SPUInstruction& instr);
    void gbh(const SPUInstruction& instr);
    void gbb(const SPUInstruction& instr);
    void fsm(const SPUInstruction& instr);
    void fsmh(const SPUInstruction& instr);
    void fsmb(const SPUInstruction& instr);
    void frest(const SPUInstruction& instr);
    void frsqest(const SPUInstruction& instr);
    void lqx(const SPUInstruction& instr);
    void rotqbybi(const SPUInstruction& instr);
    void rotqmbybi(const SPUInstruction& instr);
    void shlqbybi(const SPUInstruction& instr);
    void cbx(const SPUInstruction& instr);
    void chx(const SPUInstruction& instr);
    void cwx(const SPUInstruction& instr);
    void cdx(const SPUInstruction& instr);
    void rotqbi(const SPUInstruction& instr);
    void rotqmbi(const SPUInstruction& instr);
    void shlqbi(const SPUInstruction& instr);
    void rotqby(const SPUInstruction& instr);
    void rotqmby(const SPUInstruction& instr);
    void shlqby(const SPUInstruction& instr);
    void orx(const SPUInstruction& instr);
    void cbd(const SPUInstruction& instr);
    void chd(const SPUInstruction& instr);
    void cwd(const SPUInstruction& instr);
    void cdd(const SPUInstruction& instr);
    void rotqbii(const SPUInstruction& instr);
    void rotqmbii(const SPUInstruction& instr);
    void shlqbii(const SPUInstruction& instr);
    void rotqbyi(const SPUInstruction& instr);
    void rotqmbyi(const SPUInstruction& instr);
    void shlqbyi(const SPUInstruction& instr);
    void nop(const SPUInstruction& instr);
    void cgt(const SPUInstruction& instr);
    void xor_(const SPUInstruction& instr);
    void cgth(const SPUInstruction& instr);
    void eqv(const SPUInstruction& instr);
    void cgtb(const SPUInstruction& instr);
    void sumb(const SPUInstruction& instr);
    void hgt(const SPUInstruction& instr);
    void clz(const SPUInstruction& instr);
    void xswd(const SPUInstruction& instr);
    void xshw(const SPUInstruction& instr);
    void cntb(const SPUInstruction& instr);
    void xsbh(const SPUInstruction& instr);
    void clgt(const SPUInstruction& instr);
    void andc(const SPUInstruction& instr);
    void fcgt(const SPUInstruction& instr);
    void dfcgt(const SPUInstruction& instr);
    void fa(const SPUInstruction& instr);
    void fs(const SPUInstruction& instr);
    void fm(const SPUInstruction& instr);
    void clgth(const SPUInstruction& instr);
    void orc(const SPUInstruction& instr);
    void fcmgt(const SPUInstruction& instr);
    void dfcmgt(const SPUInstruction& instr);
    void dfa(const SPUInstruction& instr);
    void dfs(const SPUInstruction& instr);
    void dfm(const SPUInstruction& instr);
    void clgtb(const SPUInstruction& instr);
    void hlgt(const SPUInstruction& instr);
    void dfma(const SPUInstruction& instr);
    void dfms(const SPUInstruction& instr);
    void dfnms(const SPUInstruction& instr);
    void dfnma(const SPUInstruction& instr);
    void ceq(const SPUInstruction& instr);
    void mpyhhu(const SPUInstruction& instr);
    void addx(const SPUInstruction& instr);
    void sfx(const SPUInstruction& instr);
    void cgx(const SPUInstruction& instr);
    void bgx(const SPUInstruction& instr);
    void mpyhha(const SPUInstruction& instr);
    void mpyhhau(const SPUInstruction& instr);
    void fscrrd(const SPUInstruction& instr);
    void fesd(const SPUInstruction& instr);
    void frds(const SPUInstruction& instr);
    void fscrwr(const SPUInstruction& instr);
    void dftsv(const SPUInstruction& instr);
    void fceq(const SPUInstruction& instr);
    void dfceq(const SPUInstruction& instr);
    void mpy(const SPUInstruction& instr);
    void mpyh(const SPUInstruction& instr);
    void mpyhh(const SPUInstruction& instr);
    void mpys(const SPUInstruction& instr);
    void ceqh(const SPUInstruction& instr);
    void fcmeq(const SPUInstruction& instr);
    void dfcmeq(const SPUInstruction& instr);
    void mpyu(const SPUInstruction& instr);
    void ceqb(const SPUInstruction& instr);
    void fi(const SPUInstruction& instr);
    void heq(const SPUInstruction& instr);
    void cflts(const SPUInstruction& instr);
    void cfltu(const SPUInstruction& instr);
    void csflt(const SPUInstruction& instr);
    void cuflt(const SPUInstruction& instr);
    void brz(const SPUInstruction& instr);
    void stqa(const SPUInstruction& instr);
    void brnz(const SPUInstruction& instr);
    void brhz(const SPUInstruction& instr);
    void brhnz(const SPUInstruction& instr);
    void stqr(const SPUInstruction& instr);
    void bra(const SPUInstruction& instr);
    void lqa(const SPUInstruction& instr);
    void brasl(const SPUInstruction& instr);
    void br(const SPUInstruction& instr);
    void fsmbi(const SPUInstruction& instr);
    void brsl(const SPUInstruction& instr);
    void lqr(const SPUInstruction& instr);
    void il(const SPUInstruction& instr);
    void ilhu(const SPUInstruction& instr);
    void ilh(const SPUInstruction& instr);
    void iohl(const SPUInstruction& instr);
    void ori(const SPUInstruction& instr);
    void orhi(const SPUInstruction& instr);
    void orbi(const SPUInstruction& instr);
    void sfi(const SPUInstruction& instr);
    void sfhi(const SPUInstruction& instr);
    void andi(const SPUInstruction& instr);
    void andhi(const SPUInstruction& instr);
    void andbi(const SPUInstruction& instr);
    void ai(const SPUInstruction& instr);
    void ahi(const SPUInstruction& instr);
    void stqd(const SPUInstruction& instr);
    void lqd(const SPUInstruction& instr);
    void xori(const SPUInstruction& instr);
    void xorhi(const SPUInstruction& instr);
    void xorbi(const SPUInstruction& instr);
    void cgti(const SPUInstruction& instr);
    void cgthi(const SPUInstruction& instr);
    void cgtbi(const SPUInstruction& instr);
    void hgti(const SPUInstruction& instr);
    void clgti(const SPUInstruction& instr);
    void clgthi(const SPUInstruction& instr);
    void clgtbi(const SPUInstruction& instr);
    void hlgti(const SPUInstruction& instr);
    void mpyi(const SPUInstruction& instr);
    void mpyui(const SPUInstruction& instr);
    void ceqi(const SPUInstruction& instr);
    void ceqhi(const SPUInstruction& instr);
    void ceqbi(const SPUInstruction& instr);
    void heqi(const SPUInstruction& instr);
    void hbra(const SPUInstruction& instr);
    void hbrr(const SPUInstruction& instr);
    void ila(const SPUInstruction& instr);
    void selb(const SPUInstruction& instr);
    void shufb(const SPUInstruction& instr);
    void mpya(const SPUInstruction& instr);
    void fnms(const SPUInstruction& instr);
    void fma(const SPUInstruction& instr);
    void fms(const SPUInstruction& instr);
};
