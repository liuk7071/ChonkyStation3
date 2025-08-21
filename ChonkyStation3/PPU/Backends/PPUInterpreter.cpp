#include "PPUInterpreter.hpp"
#include <PlayStation3.hpp>


//#define PRINT_DEBUG_SYMBOLS
#define TRACK_CALL_STACK
//#define TRACK_STATE
//#define ENABLE_CONDITIONAL_TRACE_LOG

using Instruction = PPUTypes::Instruction;

static inline u64 rotl32(u32 v, u32 sh) { return std::rotl<u64>(v | ((u64)v << 32), sh); }
static inline u64 rotr32(u32 v, u32 sh) { return std::rotr<u64>(v | ((u64)v << 32), sh); }

PPUInterpreter::PPUInterpreter(Memory& mem, PlayStation3* ps3) : PPU(mem, ps3) {
    // Generate a rotation mask array - this code is adapted from RPCS3
    for (u32 mb = 0; mb < 64; mb++) {
        for (u32 me = 0; me < 64; me++) {
            const u64 mask = ((u64)-1 >> mb) ^ ((me >= 63) ? 0 : (u64)-1 >> (me + 1));
            rotation_mask[mb][me] = mb > me ? ~mask : mask;
        }
    }
}

void PPUInterpreter::printCallStack() {
    for (auto& i : call_stack) {
        if (known_funcs.contains(i.first))
            printf("%s called from 0x%08x\n", known_funcs[i.first].c_str(), i.second);
        else
            printf("sub_%x called from 0x%08x\n", i.first, i.second);
    }
    printf("current pc: 0x%08x\n", (u32)state.pc);
}

void PPUInterpreter::printFunctionCall() {
    auto symbol = ps3->elf_parser.getSymbol(state.pc);
    if (symbol.has_value())
        printf("[DEBUG] %s @ 0x%08llx\n", symbol.value().name.c_str(), state.pc);
}

int PPUInterpreter::step() {
    int cycles = 0;

    do {
        const u32 instr_raw = mem.read<u32>(state.pc);
        const Instruction instr = { .raw = instr_raw };
        
#ifndef CHONKYSTATION3_USER_BUILD

#ifdef ENABLE_CONDITIONAL_TRACE_LOG
    if (should_log)
        printf("%s\n", PPUDisassembler::disasm(state, instr, &mem).c_str());
#endif
#ifdef TRACK_STATE
    ps3->crash_analyzer.saveState({ state, instr });
#endif

#endif
        
        switch (instr.opc) {
                
            case G_04: {
                switch (instr.g_04_field & 0x3f) {
                        
                    case VMADDFP:       vmaddfp(instr);     break;
                    case VMHRADDSHS:    vmhraddshs(instr);  break;
                    case VMLADDUHM:     vmladduhm(instr);   break;
                    case VNMSUBFP:      vnmsubfp(instr);    break;
                    case VMSUMSHM:      vmsumshm(instr);    break;
                    case VSEL:          vsel(instr);        break;
                    case VPERM:         vperm(instr);       break;
                    case VSLDOI:        vsldoi(instr);      break;
                        
                    default:
                        switch (instr.g_04_field) {
                                
                            case VCMPEQUB:  vcmpequb(instr);    break;
                            case VADDFP:    vaddfp(instr);      break;
                            case VADDUHM:   vadduhm(instr);     break;
                            case VMULOUH:   vmulouh(instr);     break;
                            case VSUBFP:    vsubfp(instr);      break;
                            case VMRGHH:    vmrghh(instr);      break;
                            case VADDUWM:   vadduwm(instr);     break;
                            case VRLW:      vrlw(instr);        break;
                            case VCMPEQUW_:
                            case VCMPEQUW:  vcmpequw(instr);    break;
                            case VMRGHW:    vmrghw(instr);      break;
                            case VCMPEQFP_:
                            case VCMPEQFP:  vcmpeqfp(instr);    break;
                            case VSLB:      vslb(instr);        break;
                            case VREFP:     vrefp(instr);       break;
                            case VPKSHUS:   vpkshus(instr);     break;
                            case VSLH:      vslh(instr);        break;
                            case VMULOSH:   vmulosh(instr);     break;
                            case VRSQRTEFP: vrsqrtefp(instr);   break;
                            case VMRGLH:    vmrglh(instr);      break;
                            case VSLW:      vslw(instr);        break;
                            case VEXPTEFP:  vexptefp(instr);    break;
                            case VMRGLW:    vmrglw(instr);      break;
                            case VCMPGEFP_:
                            case VCMPGEFP:  vcmpgefp(instr);    break;
                            case VPKSWSS:   vpkswss(instr);     break;
                            case VCMPGTUB_:
                            case VCMPGTUB:  vcmpgtub(instr);    break;
                            case VSPLTB:    vspltb(instr);      break;
                            case VUPKHSB:   vupkhsb(instr);     break;
                            case VCMPGTUH_:
                            case VCMPGTUH:  vcmpgtuh(instr);    break;
                            case VSPLTH:    vsplth(instr);      break;
                            case VUPKHSH:   vupkhsh(instr);     break;
                            case VSRW:      vsrw(instr);        break;
                            case VCMPGTUW_:
                            case VCMPGTUW:  vcmpgtuw(instr);    break;
                            case VSPLTW:    vspltw(instr);      break;
                            case VUPKLSB:   vupklsb(instr);     break;
                            case VCMPGTFP_:
                            case VCMPGTFP:  vcmpgtfp(instr);    break;
                            case VUPKLSH:   vupklsh(instr);     break;
                            case VCFUX:     vcfux(instr);       break;
                            case VSPLTISB:  vspltisb(instr);    break;
                            case VADDSHS:   vaddshs(instr);     break;
                            case VSRAH:     vsrah(instr);       break;
                            case VMULESH:   vmulesh(instr);     break;
                            case VCFSX:     vcfsx(instr);       break;
                            case VSPLTISH:  vspltish(instr);    break;
                            case VSRAW:     vsraw(instr);       break;
                            case VCMPGTSW:  vcmpgtsw(instr);    break;
                            case VCTUXS:    vctuxs(instr);      break;
                            case VSPLTISW:  vspltisw(instr);    break;
                            case VCTSXS:    vctsxs(instr);      break;
                            case VAND:      vand(instr);        break;
                            case VMAXFP:    vmaxfp(instr);      break;
                            case VSUBUHM:   vsubuhm(instr);     break;
                            case VANDC:     vandc(instr);       break;
                            case VMINFP:    vminfp(instr);      break;
                            case VSUBUWM:   vsubuwm(instr);     break;
                            case VOR:       vor(instr);         break;
                            case VNOR:      vnor(instr);        break;
                            case MFVSCR:    mfvscr(instr);      break;
                            case MTVSCR:    mtvscr(instr);      break;
                            case VXOR:      vxor(instr);        break;
                            case VSUM4SBS:  vsum4sbs(instr);    break;
                            case VSUBSHS:   vsubshs(instr);     break;
                            case VSUMSWS:   vsumsws(instr);     break;
                                
                            default:
                                Helpers::panic("Unimplemented G_04 instruction 0x%02x (decimal: %d) (full instr: 0x%08x) @ 0x%016llx\n", (u32)instr.g_04_field, (u32)instr.g_04_field, instr.raw, state.pc);
                        }
                }
                break;
            }
            case MULLI:  mulli(instr);   break;
            case SUBFIC: subfic(instr);  break;
            case CMPLI:  cmpli(instr);   break;
            case CMPI:   cmpi(instr);    break;
            case ADDIC:  addic(instr);   break;
            case ADDIC_: addic_(instr);  break;
            case ADDI:   addi(instr);    break;
            case ADDIS:  addis(instr);   break;
            case BC:     bc(instr);      break;
            case SC:     sc(instr);      break;
            case B:      b(instr);       break;
            case G_13: {
                switch (instr.g_13_field) {
                        
                    case MCRF:      mcrf(instr);    break;
                    case BCLR:      bclr(instr);    break;
                    case CRNOR:     crnor(instr);   break;
                    case CRANDC:    crandc(instr);  break;
                    case ISYNC:     break;
                    case CRNAND:    crnand(instr);  break;
                    case CRAND:     crand(instr);  break;
                    case CRORC:     crorc(instr);   break;
                    case CROR:      cror(instr);    break;
                    case BCCTR:     bcctr(instr);   break;
                        
                    default:
                        Helpers::panic("Unimplemented G_13 instruction 0x%02x (decimal: %d) (full instr: 0x%08x) @ 0x%016llx\n", (u32)instr.g_13_field, (u32)instr.g_13_field, instr.raw, state.pc);
                }
                break;
            }
            case RLWIMI:    rlwimi(instr);  break;
            case RLWINM:    rlwinm(instr);  break;
            case RLWNM:     rlwnm(instr);   break;
            case ORI:       ori(instr);     break;
            case ORIS:      oris(instr);    break;
            case XORI:      xori(instr);    break;
            case XORIS:     xoris(instr);   break;
            case ANDI:      andi(instr);    break;
            case ANDIS:     andis(instr);   break;
            case G_1E: {
                switch (instr.g_1e_field) {
                        
                    case RLDICL_:
                    case RLDICL:    rldicl(instr);  break;
                    case RLDICR_:
                    case RLDICR:    rldicr(instr);  break;
                    case RLDIC_:
                    case RLDIC:     rldic(instr);   break;
                    case RLDIMI_:
                    case RLDIMI:    rldimi(instr);  break;
                    case RLDCL:     rldcl(instr);   break;
                        
                    default:
                        Helpers::panic("Unimplemented G_1E instruction 0x%02x (decimal: %d) (full instr: 0x%08x)\n", (u32)instr.g_1e_field, (u32)instr.g_1e_field, instr.raw);
                }
                break;
            }
            case G_1F: {
                switch (instr.g_1f_field) {
                        
                    case CMP:       cmp(instr);     break;
                        //case TW:        ps3->thread_manager.getCurrentThread()->wait(); break;
                    case LVSL:      lvsl(instr);    break;
                    case SUBFC:     subfc(instr);   break;
                    case MULHDU:    mulhdu(instr);  break;
                    case ADDC:      addc(instr);    break;
                    case MULHWU:    mulhwu(instr);  break;
                    case MFCR:      mfcr(instr);    break;
                    case LWARX:     lwarx(instr);   break;
                    case LDX:       ldx(instr);     break;
                    case LWZX:      lwzx(instr);    break;
                    case CNTLZW:    cntlzw(instr);  break;
                    case SLW:       slw(instr);     break;
                    case SLD:       sld(instr);     break;
                    case AND:       and_(instr);    break;
                    case CMPL:      cmpl(instr);    break;
                    case LVSR:      lvsr(instr);    break;
                    case SUBF:      subf(instr);    break;
                    case DCBST:     break;
                    case LWZUX:     lwzux(instr);   break;
                    case CNTLZD:    cntlzd(instr);  break;
                    case ANDC:      andc(instr);    break;
                    case LVEWX:     lvewx(instr);   break;
                    case MULHD:     mulhd(instr);   break;
                    case MULHW:     mulhw(instr);   break;
                    case LDARX:     ldarx(instr);   break;
                    case LBZX:      lbzx(instr);    break;
                    case LVX:       lvx(instr);     break;
                    case NEG:       neg(instr);     break;
                    case NOR:       nor(instr);     break;
                    case STVEBX:    stvebx(instr);  break;
                    case SUBFE:     subfe(instr);   break;
                    case ADDE:      adde(instr);    break;
                    case MTCRF:     mtcrf(instr);   break;
                    case STDX:      stdx(instr);    break;
                    case STWCX_:    stwcx(instr);   break;
                    case STWX:      stwx(instr);    break;
                    case STVEHX:    stvehx(instr);  break;
                    case STDUX:     stdux(instr);   break;
                    case STVEWX:    stvewx(instr);  break;
                    case ADDZE:     addze(instr);   break;
                    case STDCX_:    stdcx(instr);   break;
                    case STBX:      stbx(instr);    break;
                    case STVX:      stvx(instr);    break;
                    case MULLD:     mulld(instr);   break;
                    case MULLW:     mullw(instr);   break;
                    case STBUX:     stbux(instr);   break;
                    case DCBTST:    break;
                    case ADD:       add(instr);     break;
                    case DCBT:      break;
                    case LHZX:      lhzx(instr);    break;
                    case LHZUX:     lhzux(instr);    break;
                    case XOR:       xor_(instr);    break;
                    case MFSPR:     mfspr(instr);   break;
                    case DST:       break;
                    case MFTB:      mftb(instr);    break;
                    case DSTST:     break;
                    case STHX:      sthx(instr);    break;
                    case ORC:       orc(instr);     break;
                    case OR:        or_(instr);     break;
                    case DIVDU:     divdu(instr);   break;
                    case DIVWU:     divwu(instr);   break;
                    case MTSPR:     mtspr(instr);   break;
                    case NAND:      nand(instr);    break;
                    case DIVD:      divd(instr);    break;
                    case DIVW:      divw(instr);    break;
                    case LVLX:      lvlx(instr);    break;
                    case LWBRX:     lwbrx(instr);   break;
                    case LFSX:      lfsx(instr);    break;
                    case SRW:       srw(instr);     break;
                    case SRD:       srd(instr);     break;
                    case LVRX:      lvrx(instr);    break;
                    case SYNC:      break;
                    case LFDX:      lfdx(instr);    break;
                    case STVLX:     stvlx(instr);   break;
                    case STFSX:     stfsx(instr);   break;
                    case STVRX:     stvrx(instr);   break;
                    case STFDX:     stfdx(instr);   break;
                    case LHBRX:     lhbrx(instr);   break;
                    case SRAW:      sraw(instr);    break;
                    case SRAD:      srad(instr);    break;
                    case DSS:       break;
                    case SRAWI:     srawi(instr);   break;
                    case SRADI1:
                    case SRADI2:    sradi(instr);   break;
                    case EIEIO:     break;
                    case EXTSH:     extsh(instr);   break;
                    case EXTSB:     extsb(instr);   break;
                    case EXTSW:     extsw(instr);   break;
                    case STFIWX:    stfiwx(instr);  break;
                    case DCBZ:      dcbz(instr);    break;
                        
                    default:
                        Helpers::panic("Unimplemented G_1F instruction 0x%03x (decimal: %d) (full instr: 0x%08x) @ 0x%016llx\n", (u32)instr.g_1f_field, (u32)instr.g_1f_field, instr.raw, state.pc);
                }
                break;
            }
            case LWZ:   lwz(instr);     break;
            case LWZU:  lwzu(instr);    break;
            case LBZ:   lbz(instr);     break;
            case LBZU:  lbzu(instr);    break;
            case STW:   stw(instr);     break;
            case STWU:  stwu(instr);    break;
            case STB:   stb(instr);     break;
            case STBU:  stbu(instr);    break;
            case LHZ:   lhz(instr);     break;
            case LHZU:  lhzu(instr);    break;
            case STH:   sth(instr);     break;
            case STHU:  sthu(instr);    break;
            case LFS:   lfs(instr);     break;
            case LFSU:  lfsu(instr);     break;
            case LFD:   lfd(instr);     break;
            case LFDU:  lfdu(instr);    break;
            case STFS:  stfs(instr);    break;
            case STFSU: stfsu(instr);   break;
            case STFD:  stfd(instr);    break;
            case STFDU: stfdu(instr);   break;
            case G_3A: {
                switch (instr.g_3a_field) {
                        
                    case LD:    ld(instr);      break;
                    case LDU:   ldu(instr);     break;
                    case LWA:   lwa(instr);     break;
                        
                    default:
                        Helpers::panic("Unimplemented G_3A instruction 0x%02x (decimal: %d) (full instr: 0x%08x)\n", (u32)instr.g_3a_field, (u32)instr.g_3a_field, instr.raw);
                }
                break;
            }
            case G_3B: {
                switch (instr.g_3b_field) {
                        
                    case FDIVS:     fdivs(instr);   break;
                    case FSUBS:     fsubs(instr);   break;
                    case FADDS:     fadds(instr);   break;
                    case FSQRTS:    fsqrts(instr);  break;
                    case FMULS:     fmuls(instr);   break;
                    case FMSUBS:    fmsubs(instr);  break;
                    case FMADDS:    fmadds(instr);  break;
                    case FNMSUBS:   fnmsubs(instr); break;
                    case FNMADDS:   fnmadds(instr); break;
                        
                    default:
                        Helpers::panic("Unimplemented G_3B instruction 0x%02x (decimal: %d) (full instr: 0x%08x)\n", (u32)instr.g_3b_field, (u32)instr.g_3b_field, instr.raw);
                }
                break;
            }
            case G_3E: {
                switch (instr.g_3e_field) {
                        
                    case STD:   std(instr);     break;
                    case STDU:  stdu(instr);    break;
                        
                    default:
                        Helpers::panic("Unimplemented G_3E instruction 0x%02x (decimal: %d) (full instr: 0x%08x)\n", (u32)instr.g_3e_field, (u32)instr.g_3e_field, instr.raw);
                }
                break;
            }
            case G_3F: {
                switch (instr.g_3f_field & 0x1f) {
                        
                    case FSEL:      fsel(instr);    break;
                    case FMUL:      fmul(instr);    break;
                    case FRSQRTE:   frsqrte(instr); break;
                    case FMSUB:     fmsub(instr);   break;
                    case FMADD:     fmadd(instr);   break;
                    case FNMSUB:    fnmsub(instr);  break;
                    case FNMADD:    fnmadd(instr);  break;
                        
                    default:
                        switch (instr.g_3f_field) {
                                
                            case MFFS:      mffs(instr);    break;
                            case MTFSF:     mtfsf(instr);   break;
                            case FCMPU:     fcmpu(instr);   break;
                            case FRSP:      frsp(instr);    break;
                            case FCTIW:
                            case FCTIWZ:    fctiwz(instr);  break;
                            case FDIV:      fdiv(instr);    break;
                            case FSUB:      fsub(instr);    break;
                            case FADD:      fadd(instr);    break;
                            case FSQRT:     fsqrt(instr);   break;
                            case FMR:       fmr(instr);     break;
                            case FNEG:      fneg(instr);    break;
                            case FNABS:     fnabs(instr);   break;
                            case FABS:      fabs_(instr);   break;
                            case FCTID:     fctid(instr);   break;
                            case FCTIDZ:    fctidz(instr);  break;
                            case FCFID:     fcfid(instr);   break;
                                
                            default:
                                Helpers::panic("Unimplemented G_3F instruction 0x%02x (decimal: %d) (full instr: 0x%08x) @ 0x%016llx\n", (u32)instr.g_3f_field, (u32)instr.g_3f_field, instr.raw, state.pc);
                        }
                }
                break;
            }
                
            default:
                Helpers::panic("Unimplemented opcode 0x%02x (decimal: %d) (full instr: 0x%08x) @ 0x%016llx\n", (u32)instr.opc, (u32)instr.opc, instr.raw, state.pc);
        }
        
        state.pc += 4;
        if (cycles++ > 2048) should_break = true;
    } while (!should_break);
    
    should_break = false;
    return cycles;
}

// Main

void PPUInterpreter::mulli(const Instruction& instr) {
    const s64 si = (s64)(s16)instr.si;
    state.gprs[instr.rt] = (s64)state.gprs[instr.ra] * si;
}

void PPUInterpreter::subfic(const Instruction& instr) {
    const auto a = state.gprs[instr.ra];
    const s64 b = (s64)(s16)instr.si;
    const auto res1 = ~a + b;
    const auto res2 = res1 + 1;

    // See if ~a + b carried
    const bool carry1 = res1 < b;
    // See if adding 1 carried
    const bool carry2 = res2 < res1;

    state.xer.ca = (carry1 || carry2) ? 1 : 0;
    state.gprs[instr.rt] = res2;
}

void PPUInterpreter::cmpli(const Instruction& instr) {
    const u64 a = state.gprs[instr.ra];
    const u64 b = instr.ui;
    if (instr.l)
        state.cr.compareAndUpdateCRField<u64>(instr.bf, a, b, 0);
    else
        state.cr.compareAndUpdateCRField<u32>(instr.bf, a, b, 0);
}

void PPUInterpreter::cmpi(const Instruction& instr) {
    const s64 a = (s64)state.gprs[instr.ra];
    const s64 b = (s64)(s16)instr.si;
    if (instr.l)
        state.cr.compareAndUpdateCRField<s64>(instr.bf, a, b, 0);
    else
        state.cr.compareAndUpdateCRField<s32>(instr.bf, a, b, 0);
}

void PPUInterpreter::addic(const Instruction& instr) {
    const auto a = state.gprs[instr.ra];
    const s64 b = (s64)(s16)instr.si;
    const auto res = a + b;
    state.xer.ca = res < a;
    state.gprs[instr.rt] = res;
}

void PPUInterpreter::addic_(const Instruction& instr) {
    const auto a = state.gprs[instr.ra];
    const s64 b = (s64)(s16)instr.si;
    const auto res = a + b;
    state.xer.ca = res < a;
    state.gprs[instr.rt] = res;
    if (instr.rc)
        state.cr.compareAndUpdateCRField<s64>(0, state.gprs[instr.rt], 0);
}

void PPUInterpreter::addi(const Instruction& instr) {
    state.gprs[instr.rt] = (instr.ra ? state.gprs[instr.ra] : 0) + (s64)(s16)instr.si;
}

void PPUInterpreter::addis(const Instruction& instr) {
    state.gprs[instr.rt] = (instr.ra ? state.gprs[instr.ra] : 0) + (s64)(s32)(instr.si << 16);
}

void PPUInterpreter::bc(const Instruction& instr) {
    const s64 sbd = (s64)(s16)(instr.bd << 2);
    if (instr.lk)  // Link
        state.lr = state.pc + 4;

    if (branchCondition(instr.bo, instr.bi)) {
        state.pc = (instr.aa == 1) ? sbd : (state.pc + sbd);
#ifdef PRINT_DEBUG_SYMBOLS
        printFunctionCall();
#endif
#if !defined(CHONKYSTATION3_USER_BUILD) && defined(TRACK_CALL_STACK)
        if (instr.lk)
            call_stack.push_back({ state.pc, state.lr - 4 });
#endif
        state.pc -= 4;
    }
}

void PPUInterpreter::sc(const Instruction& instr) {
    should_break = true;
    ps3->syscall.doSyscall(true);
}

void PPUInterpreter::b(const Instruction& instr) {
    const s64 sli = (s64)(s32)(instr.li << 8) >> 6;    // ((instr.li << 2) << 6) >> 6

    if (instr.lk)  // Link
        state.lr = state.pc + 4;

    state.pc = (instr.aa == 1) ? sli : (state.pc + sli);
#ifdef PRINT_DEBUG_SYMBOLS
    printFunctionCall();
#endif
#if !defined(CHONKYSTATION3_USER_BUILD) && defined(TRACK_CALL_STACK)
    if (instr.lk)
        call_stack.push_back({ state.pc, state.lr - 4 });
#endif
    state.pc -= 4;
}

void PPUInterpreter::rlwimi(const Instruction& instr) {
    const u64 mask = rotation_mask[32 + instr.mb_5][32 + instr.me_5];
    state.gprs[instr.ra] = (state.gprs[instr.ra] & ~mask) | (rotl32(state.gprs[instr.rs], instr.sh) & mask);
    if (instr.rc)
        state.cr.compareAndUpdateCRField<s32>(0, state.gprs[instr.ra], 0);
}

void PPUInterpreter::rlwinm(const Instruction& instr) {
    const u64 mask = rotation_mask[32 + instr.mb_5][32 + instr.me_5];
    state.gprs[instr.ra] = rotl32(state.gprs[instr.rs], instr.sh) & mask;
    if (instr.rc)
        state.cr.compareAndUpdateCRField<s32>(0, state.gprs[instr.ra], 0);
}

void PPUInterpreter::rlwnm(const Instruction& instr) {
    const u64 mask = rotation_mask[32 + instr.mb_5][32 + instr.me_5];
    const auto rot = state.gprs[instr.rb] & 0x1f;
    state.gprs[instr.ra] = rotl32(state.gprs[instr.rs], rot) & mask;
    if (instr.rc)
        state.cr.compareAndUpdateCRField<s32>(0, state.gprs[instr.ra], 0);
}

void PPUInterpreter::ori(const Instruction& instr) {
    state.gprs[instr.ra] = state.gprs[instr.rs] | instr.ui;
}

void PPUInterpreter::oris(const Instruction& instr) {
    state.gprs[instr.ra] = state.gprs[instr.rs] | ((u64)instr.ui << 16u);
}

void PPUInterpreter::xori(const Instruction& instr) {
    state.gprs[instr.ra] = state.gprs[instr.rs] ^ instr.ui;
}

void PPUInterpreter::xoris(const Instruction& instr) {
    state.gprs[instr.ra] = state.gprs[instr.rs] ^ ((u64)instr.ui << 16u);
}

void PPUInterpreter::andi(const Instruction& instr) {
    state.gprs[instr.ra] = state.gprs[instr.rs] & instr.ui;
    state.cr.compareAndUpdateCRField<s64>(0, state.gprs[instr.ra], 0);
}

void PPUInterpreter::andis(const Instruction& instr) {
    state.gprs[instr.ra] = state.gprs[instr.rs] & ((u64)instr.ui << 16u);
    state.cr.compareAndUpdateCRField<s64>(0, state.gprs[instr.ra], 0);
}

void PPUInterpreter::lwz(const Instruction& instr) {
    const s32 sd = (s32)(s16)instr.d;
    const u32 addr = (instr.ra == 0) ? sd : state.gprs[instr.ra] + sd;
    state.gprs[instr.rt] = mem.read<u32>(addr);
}

void PPUInterpreter::lwzu(const Instruction& instr) {
    const s32 sd = (s32)(s16)instr.d;
    const u64 addr = state.gprs[instr.ra] + sd;
    state.gprs[instr.rt] = mem.read<u32>(addr & 0xffffffff);
    state.gprs[instr.ra] = addr;    // Update
}

void PPUInterpreter::lbz(const Instruction& instr) {
    const s32 sd = (s32)(s16)instr.d;
    const u32 addr = (instr.ra == 0) ? sd : state.gprs[instr.ra] + sd;
    state.gprs[instr.rt] = mem.read<u8>(addr);
}

void PPUInterpreter::lbzu(const Instruction& instr) {
    const s32 sd = (s32)(s16)instr.d;
    const u64 addr = state.gprs[instr.ra] + sd;
    state.gprs[instr.rt] = mem.read<u8>(addr & 0xffffffff);
    state.gprs[instr.ra] = addr;    // Update
}

void PPUInterpreter::stw(const Instruction& instr) {
    const s32 sd = (s32)(s16)instr.d;
    const u32 addr = (instr.ra == 0) ? sd : state.gprs[instr.ra] + sd;
    mem.write<u32>(addr, state.gprs[instr.rs]);
}

void PPUInterpreter::stwu(const Instruction& instr) {
    const s32 sd = (s32)(s16)instr.d;
    const u64 addr = state.gprs[instr.ra] + sd;
    mem.write<u32>(addr & 0xffffffff, state.gprs[instr.rs]);
    state.gprs[instr.ra] = addr;    // Update
}

void PPUInterpreter::stb(const Instruction& instr) {
    const s32 sd = (s32)(s16)instr.d;
    const u32 addr = (instr.ra == 0) ? sd : state.gprs[instr.ra] + sd;
    mem.write<u8>(addr, state.gprs[instr.rs]);
}

void PPUInterpreter::stbu(const Instruction& instr) {
    const s32 sd = (s32)(s16)instr.d;
    const u64 addr = state.gprs[instr.ra] + sd;
    mem.write<u8>(addr & 0xffffffff, state.gprs[instr.rs]);
    state.gprs[instr.ra] = addr;    // Update
}

void PPUInterpreter::lhz(const Instruction& instr) {
    const s32 sd = (s32)(s16)instr.d;
    const u32 addr = (instr.ra == 0) ? sd : state.gprs[instr.ra] + sd;
    state.gprs[instr.rt] = mem.read<u16>(addr);
}

void PPUInterpreter::lhzu(const Instruction& instr) {
    const s32 sd = (s32)(s16)instr.d;
    const u64 addr = state.gprs[instr.ra] + sd;
    state.gprs[instr.rt] = mem.read<u16>(addr & 0xffffffff);
    state.gprs[instr.ra] = addr;    // Update
}

void PPUInterpreter::sth(const Instruction& instr) {
    const s32 sd = (s32)(s16)instr.d;
    const u32 addr = (instr.ra == 0) ? sd : state.gprs[instr.ra] + sd;
    mem.write<u16>(addr, state.gprs[instr.rs]);
}

void PPUInterpreter::sthu(const Instruction& instr) {
    const s32 sd = (s32)(s16)instr.d;
    const u64 addr = state.gprs[instr.ra] + sd;
    mem.write<u16>(addr & 0xffffffff, state.gprs[instr.rs]);
    state.gprs[instr.ra] = addr;    // Update
}

void PPUInterpreter::lfs(const Instruction& instr) {
    const s32 sd = (s32)(s16)instr.d;
    const u32 addr = (instr.ra == 0) ? sd : state.gprs[instr.ra] + sd;
    u32 v = mem.read<u32>(addr);
    state.fprs[instr.frt] = reinterpret_cast<float&>(v);
}

void PPUInterpreter::lfsu(const Instruction& instr) {
    const s32 sd = (s32)(s16)instr.d;
    const u64 addr = state.gprs[instr.ra] + sd;
    u32 v = mem.read<u32>(addr & 0xffffffff);
    state.fprs[instr.frt] = reinterpret_cast<float&>(v);
    state.gprs[instr.ra] = addr;    // Update
}

void PPUInterpreter::lfd(const Instruction& instr) {
    const s32 sd = (s32)(s16)instr.d;
    const u32 addr = (instr.ra == 0) ? sd : state.gprs[instr.ra] + sd;
    u64 v = mem.read<u64>(addr);
    state.fprs[instr.frt] = reinterpret_cast<double&>(v);
}

void PPUInterpreter::lfdu(const Instruction& instr) {
    const s32 sd = (s32)(s16)instr.d;
    const u64 addr = state.gprs[instr.ra] + sd;
    u64 v = mem.read<u64>(addr & 0xffffffff);
    state.fprs[instr.frt] = reinterpret_cast<double&>(v);
    state.gprs[instr.ra] = addr;    // Update
}

void PPUInterpreter::stfs(const Instruction& instr) {
    const s32 sd = (s32)(s16)instr.d;
    const u32 addr = (instr.ra == 0) ? sd : state.gprs[instr.ra] + sd;
    float v = (float)state.fprs[instr.frs];
    mem.write<u32>(addr, reinterpret_cast<u32&>(v));
}

void PPUInterpreter::stfsu(const Instruction& instr) {
    const s32 sd = (s32)(s16)instr.d;
    const u64 addr = state.gprs[instr.ra] + sd;
    float v = (float)state.fprs[instr.frs];
    mem.write<u32>(addr & 0xffffffff, reinterpret_cast<u32&>(v));
    state.gprs[instr.ra] = addr;    // Update
}

void PPUInterpreter::stfd(const Instruction& instr) {
    const s32 sd = (s32)(s16)instr.d;
    const u32 addr = (instr.ra == 0) ? sd : state.gprs[instr.ra] + sd;
    mem.write<u64>(addr, reinterpret_cast<u64&>(state.fprs[instr.frs]));
}

void PPUInterpreter::stfdu(const Instruction& instr) {
    const s32 sd = (s32)(s16)instr.d;
    const u64 addr = state.gprs[instr.ra] + sd;
    mem.write<u64>(addr & 0xffffffff, reinterpret_cast<u64&>(state.fprs[instr.frs]));
    state.gprs[instr.ra] = addr;    // Update
}

// G_04

void PPUInterpreter::vcmpequb(const Instruction& instr) {
    u8 all_equal = 0x8;
    u8 none_equal = 0x2;

    for (int i = 0; i < 16; i++) {
        if (state.vrs[instr.va].b[i] == state.vrs[instr.vb].b[i]) {
            state.vrs[instr.vd].b[i] = 0xff;
            none_equal = 0;
        }
        else {
            state.vrs[instr.vd].b[i] = 0;
            all_equal = 0;
        }
    }

    if (instr.rc_v)
        state.cr.setCRField(6, all_equal | none_equal);
}

void PPUInterpreter::vmsumshm(const Instruction& instr) {
    for (int i = 0; i < 4; i++) {
        const s32 h1 = (s16)state.vrs[instr.va].h[i * 2 + 0] * (s16)state.vrs[instr.vb].h[i * 2 + 0];
        const s32 h2 = (s16)state.vrs[instr.va].h[i * 2 + 1] * (s16)state.vrs[instr.vb].h[i * 2 + 1];
        const s32 sum = h1 + h2;
        state.vrs[instr.vd].w[i] = state.vrs[instr.vc].w[i] + sum;
    }
}

void PPUInterpreter::vsel(const Instruction& instr) {
    // Where vc is 0, we take va, so we invert the mask and do va & ~vc
    state.vrs[instr.vd].dw[0] = (state.vrs[instr.va].dw[0] & ~state.vrs[instr.vc].dw[0]) | (state.vrs[instr.vb].dw[0] & state.vrs[instr.vc].dw[0]);
    state.vrs[instr.vd].dw[1] = (state.vrs[instr.va].dw[1] & ~state.vrs[instr.vc].dw[1]) | (state.vrs[instr.vb].dw[1] & state.vrs[instr.vc].dw[1]);
}

void PPUInterpreter::vperm(const Instruction& instr) {
    u8 src[32];
    for (int i = 0; i < 16; i++) src[i +  0] = state.vrs[instr.vb].b[i];
    for (int i = 0; i < 16; i++) src[i + 16] = state.vrs[instr.va].b[i];
    
    // The index is in the usual PPC reversed byte order, so we need to flip it with 31 - 
    for (int i = 0; i < 16; i++) {
        const auto idx = state.vrs[instr.vc].b[i] & 0x1f;
        state.vrs[instr.vd].b[i] = src[31 - idx];
    }
}

void PPUInterpreter::vsldoi(const Instruction& instr) {
    u8 src[32];
    for (int i = 0; i < 16; i++) src[i + 0] = state.vrs[instr.vb].b[i];
    for (int i = 0; i < 16; i++) src[i + 16] = state.vrs[instr.va].b[i];

    for (int i = 0; i < 16; i++) state.vrs[instr.vd].b[15 - i] = src[31 - (instr.shb + i)];
}

void PPUInterpreter::vmaddfp(const Instruction& instr) {
    state.vrs[instr.vd].f[0] = state.vrs[instr.va].f[0] * state.vrs[instr.vc].f[0] + state.vrs[instr.vb].f[0];
    state.vrs[instr.vd].f[1] = state.vrs[instr.va].f[1] * state.vrs[instr.vc].f[1] + state.vrs[instr.vb].f[1];
    state.vrs[instr.vd].f[2] = state.vrs[instr.va].f[2] * state.vrs[instr.vc].f[2] + state.vrs[instr.vb].f[2];
    state.vrs[instr.vd].f[3] = state.vrs[instr.va].f[3] * state.vrs[instr.vc].f[3] + state.vrs[instr.vb].f[3];
}

void PPUInterpreter::vnmsubfp(const Instruction& instr) {
    state.vrs[instr.vd].f[0] = -(state.vrs[instr.va].f[0] * state.vrs[instr.vc].f[0] - state.vrs[instr.vb].f[0]);
    state.vrs[instr.vd].f[1] = -(state.vrs[instr.va].f[1] * state.vrs[instr.vc].f[1] - state.vrs[instr.vb].f[1]);
    state.vrs[instr.vd].f[2] = -(state.vrs[instr.va].f[2] * state.vrs[instr.vc].f[2] - state.vrs[instr.vb].f[2]);
    state.vrs[instr.vd].f[3] = -(state.vrs[instr.va].f[3] * state.vrs[instr.vc].f[3] - state.vrs[instr.vb].f[3]);
}

void PPUInterpreter::vaddfp(const Instruction& instr) {
    state.vrs[instr.vd].f[0] = state.vrs[instr.va].f[0] + state.vrs[instr.vb].f[0];
    state.vrs[instr.vd].f[1] = state.vrs[instr.va].f[1] + state.vrs[instr.vb].f[1];
    state.vrs[instr.vd].f[2] = state.vrs[instr.va].f[2] + state.vrs[instr.vb].f[2];
    state.vrs[instr.vd].f[3] = state.vrs[instr.va].f[3] + state.vrs[instr.vb].f[3];
}

void PPUInterpreter::vmhraddshs(const Instruction& instr) {
    for (int i = 0; i < 8; i++) {
        s32 res = (s32)(s16)state.vrs[instr.va].h[i] * (s32)(s16)state.vrs[instr.vb].h[i] + state.vrs[instr.vc].h[i] + 0x4000;
        if (res > 0x7fff) res = 0x7fff;
        else if (res < -0x8000) res = -0x8000;
        state.vrs[instr.vd].h[i] = res;
    }
}

void PPUInterpreter::vmladduhm(const Instruction& instr) {
    state.vrs[instr.vd].h[0] = state.vrs[instr.va].h[0] * state.vrs[instr.vb].h[0] + state.vrs[instr.vc].h[0];
    state.vrs[instr.vd].h[1] = state.vrs[instr.va].h[1] * state.vrs[instr.vb].h[1] + state.vrs[instr.vc].h[1];
    state.vrs[instr.vd].h[2] = state.vrs[instr.va].h[2] * state.vrs[instr.vb].h[2] + state.vrs[instr.vc].h[2];
    state.vrs[instr.vd].h[3] = state.vrs[instr.va].h[3] * state.vrs[instr.vb].h[3] + state.vrs[instr.vc].h[3];
    state.vrs[instr.vd].h[4] = state.vrs[instr.va].h[4] * state.vrs[instr.vb].h[4] + state.vrs[instr.vc].h[4];
    state.vrs[instr.vd].h[5] = state.vrs[instr.va].h[5] * state.vrs[instr.vb].h[5] + state.vrs[instr.vc].h[5];
    state.vrs[instr.vd].h[6] = state.vrs[instr.va].h[6] * state.vrs[instr.vb].h[6] + state.vrs[instr.vc].h[6];
    state.vrs[instr.vd].h[7] = state.vrs[instr.va].h[7] * state.vrs[instr.vb].h[7] + state.vrs[instr.vc].h[7];
}

void PPUInterpreter::vadduhm(const Instruction& instr) {
    state.vrs[instr.vd].h[0] = state.vrs[instr.va].h[0] + state.vrs[instr.vb].h[0];
    state.vrs[instr.vd].h[1] = state.vrs[instr.va].h[1] + state.vrs[instr.vb].h[1];
    state.vrs[instr.vd].h[2] = state.vrs[instr.va].h[2] + state.vrs[instr.vb].h[2];
    state.vrs[instr.vd].h[3] = state.vrs[instr.va].h[3] + state.vrs[instr.vb].h[3];
    state.vrs[instr.vd].h[4] = state.vrs[instr.va].h[4] + state.vrs[instr.vb].h[4];
    state.vrs[instr.vd].h[5] = state.vrs[instr.va].h[5] + state.vrs[instr.vb].h[5];
    state.vrs[instr.vd].h[6] = state.vrs[instr.va].h[6] + state.vrs[instr.vb].h[6];
    state.vrs[instr.vd].h[7] = state.vrs[instr.va].h[7] + state.vrs[instr.vb].h[7];
}

void PPUInterpreter::vmulouh(const Instruction& instr) {
    // Odd actually means even for us because of the usual PPC bs
    state.vrs[instr.vd].w[0] = (u32)state.vrs[instr.va].h[0] * (u32)state.vrs[instr.vb].h[0];
    state.vrs[instr.vd].w[1] = (u32)state.vrs[instr.va].h[2] * (u32)state.vrs[instr.vb].h[2];
    state.vrs[instr.vd].w[2] = (u32)state.vrs[instr.va].h[4] * (u32)state.vrs[instr.vb].h[4];
    state.vrs[instr.vd].w[3] = (u32)state.vrs[instr.va].h[6] * (u32)state.vrs[instr.vb].h[6];
}

void PPUInterpreter::vsubfp(const Instruction& instr) {
    state.vrs[instr.vd].f[0] = state.vrs[instr.va].f[0] - state.vrs[instr.vb].f[0];
    state.vrs[instr.vd].f[1] = state.vrs[instr.va].f[1] - state.vrs[instr.vb].f[1];
    state.vrs[instr.vd].f[2] = state.vrs[instr.va].f[2] - state.vrs[instr.vb].f[2];
    state.vrs[instr.vd].f[3] = state.vrs[instr.va].f[3] - state.vrs[instr.vb].f[3];
}

void PPUInterpreter::vmrghh(const Instruction& instr) {
    const VR a = state.vrs[instr.va];
    const VR b = state.vrs[instr.vb];

    state.vrs[instr.vd].h[0] = b.h[4];
    state.vrs[instr.vd].h[1] = a.h[4];
    state.vrs[instr.vd].h[2] = b.h[5];
    state.vrs[instr.vd].h[3] = a.h[5];
    state.vrs[instr.vd].h[4] = b.h[6];
    state.vrs[instr.vd].h[5] = a.h[6];
    state.vrs[instr.vd].h[6] = b.h[7];
    state.vrs[instr.vd].h[7] = a.h[7];
}

void PPUInterpreter::vadduwm(const Instruction& instr) {
    state.vrs[instr.vd].w[0] = state.vrs[instr.va].w[0] + state.vrs[instr.vb].w[0];
    state.vrs[instr.vd].w[1] = state.vrs[instr.va].w[1] + state.vrs[instr.vb].w[1];
    state.vrs[instr.vd].w[2] = state.vrs[instr.va].w[2] + state.vrs[instr.vb].w[2];
    state.vrs[instr.vd].w[3] = state.vrs[instr.va].w[3] + state.vrs[instr.vb].w[3];
}

void PPUInterpreter::vrlw(const Instruction& instr) {
    state.vrs[instr.vd].w[0] = std::rotl<u32>(state.vrs[instr.va].w[0], state.vrs[instr.vb].w[0] & 0x1f);
    state.vrs[instr.vd].w[1] = std::rotl<u32>(state.vrs[instr.va].w[1], state.vrs[instr.vb].w[1] & 0x1f);
    state.vrs[instr.vd].w[2] = std::rotl<u32>(state.vrs[instr.va].w[2], state.vrs[instr.vb].w[2] & 0x1f);
    state.vrs[instr.vd].w[3] = std::rotl<u32>(state.vrs[instr.va].w[3], state.vrs[instr.vb].w[3] & 0x1f);
}

void PPUInterpreter::vcmpequw(const Instruction& instr) {
    u8 all_equal = 0x8;
    u8 none_equal = 0x2;

    for (int i = 0; i < 4; i++) {
        if (state.vrs[instr.va].w[i] == state.vrs[instr.vb].w[i]) {
            state.vrs[instr.vd].w[i] = 0xffffffff;
            none_equal = 0;
        }
        else {
            state.vrs[instr.vd].w[i] = 0;
            all_equal = 0;
        }
    }

    if (instr.rc_v)
        state.cr.setCRField(6, all_equal | none_equal);
}

void PPUInterpreter::vmrghw(const Instruction& instr) {
    const VR a = state.vrs[instr.va];
    const VR b = state.vrs[instr.vb];

    state.vrs[instr.vd].w[0] = b.w[2];
    state.vrs[instr.vd].w[1] = a.w[2];
    state.vrs[instr.vd].w[2] = b.w[3];
    state.vrs[instr.vd].w[3] = a.w[3];
}

void PPUInterpreter::vcmpeqfp(const Instruction& instr) {
    u8 all_equal = 0x8;
    u8 none_equal = 0x2;

    for (int i = 0; i < 4; i++) {
        if (state.vrs[instr.va].f[i] == state.vrs[instr.vb].f[i]) {
            state.vrs[instr.vd].w[i] = 0xffffffff;
            none_equal = 0;
        }
        else {
            state.vrs[instr.vd].w[i] = 0;
            all_equal = 0;
        }
    }

    if (instr.rc_v)
        state.cr.setCRField(6, all_equal | none_equal);
}

void PPUInterpreter::vslb(const Instruction& instr) {
    for (int i = 0; i < 16; i++)
        state.vrs[instr.vd].b[i] = state.vrs[instr.va].b[i] << (state.vrs[instr.vb].b[i] & 7);
}

void PPUInterpreter::vrefp(const Instruction& instr) {
    state.vrs[instr.vd].f[0] = 1.0f / state.vrs[instr.vb].f[0];
    state.vrs[instr.vd].f[1] = 1.0f / state.vrs[instr.vb].f[1];
    state.vrs[instr.vd].f[2] = 1.0f / state.vrs[instr.vb].f[2];
    state.vrs[instr.vd].f[3] = 1.0f / state.vrs[instr.vb].f[3];
}

void PPUInterpreter::vpkshus(const Instruction& instr) {
    for (int i = 0; i < 8; i++) {
        state.vrs[instr.vd].b[i] = std::clamp<s16>(state.vrs[instr.vb].h[i], 0, UINT8_MAX);
    }
    for (int i = 0; i < 8; i++) {
        state.vrs[instr.vd].b[8 + i] = std::clamp<s16>(state.vrs[instr.va].h[i], 0, UINT8_MAX);
    }
}

void PPUInterpreter::vslh(const Instruction& instr) {
    state.vrs[instr.vd].h[0] = state.vrs[instr.va].h[0] << (state.vrs[instr.vb].h[0] & 0xf);
    state.vrs[instr.vd].h[1] = state.vrs[instr.va].h[1] << (state.vrs[instr.vb].h[1] & 0xf);
    state.vrs[instr.vd].h[2] = state.vrs[instr.va].h[2] << (state.vrs[instr.vb].h[2] & 0xf);
    state.vrs[instr.vd].h[3] = state.vrs[instr.va].h[3] << (state.vrs[instr.vb].h[3] & 0xf);
    state.vrs[instr.vd].h[4] = state.vrs[instr.va].h[4] << (state.vrs[instr.vb].h[4] & 0xf);
    state.vrs[instr.vd].h[5] = state.vrs[instr.va].h[5] << (state.vrs[instr.vb].h[5] & 0xf);
    state.vrs[instr.vd].h[6] = state.vrs[instr.va].h[6] << (state.vrs[instr.vb].h[6] & 0xf);
    state.vrs[instr.vd].h[7] = state.vrs[instr.va].h[7] << (state.vrs[instr.vb].h[7] & 0xf);
}

void PPUInterpreter::vmulosh(const Instruction& instr) {
    // Odd actually means even for us because of the usual PPC bs
    state.vrs[instr.vd].w[0] = (s32)(s16)state.vrs[instr.va].h[0] * (s32)(s16)state.vrs[instr.vb].h[0];
    state.vrs[instr.vd].w[1] = (s32)(s16)state.vrs[instr.va].h[2] * (s32)(s16)state.vrs[instr.vb].h[2];
    state.vrs[instr.vd].w[2] = (s32)(s16)state.vrs[instr.va].h[4] * (s32)(s16)state.vrs[instr.vb].h[4];
    state.vrs[instr.vd].w[3] = (s32)(s16)state.vrs[instr.va].h[6] * (s32)(s16)state.vrs[instr.vb].h[6];
}

void PPUInterpreter::vrsqrtefp(const Instruction& instr) {
    state.vrs[instr.vd].f[0] = 1.0f / sqrtf(state.vrs[instr.vb].f[0]);
    state.vrs[instr.vd].f[1] = 1.0f / sqrtf(state.vrs[instr.vb].f[1]);
    state.vrs[instr.vd].f[2] = 1.0f / sqrtf(state.vrs[instr.vb].f[2]);
    state.vrs[instr.vd].f[3] = 1.0f / sqrtf(state.vrs[instr.vb].f[3]);
}

void PPUInterpreter::vmrglh(const Instruction& instr) {
    const VR a = state.vrs[instr.va];
    const VR b = state.vrs[instr.vb];

    state.vrs[instr.vd].h[0] = b.h[0];
    state.vrs[instr.vd].h[1] = a.h[0];
    state.vrs[instr.vd].h[2] = b.h[1];
    state.vrs[instr.vd].h[3] = a.h[1];
    state.vrs[instr.vd].h[4] = b.h[2];
    state.vrs[instr.vd].h[5] = a.h[2];
    state.vrs[instr.vd].h[6] = b.h[3];
    state.vrs[instr.vd].h[7] = a.h[3];
}

void PPUInterpreter::vslw(const Instruction& instr) {
    state.vrs[instr.vd].w[0] = state.vrs[instr.va].w[0] << (state.vrs[instr.vb].w[0] & 0x1f);
    state.vrs[instr.vd].w[1] = state.vrs[instr.va].w[1] << (state.vrs[instr.vb].w[1] & 0x1f);
    state.vrs[instr.vd].w[2] = state.vrs[instr.va].w[2] << (state.vrs[instr.vb].w[2] & 0x1f);
    state.vrs[instr.vd].w[3] = state.vrs[instr.va].w[3] << (state.vrs[instr.vb].w[3] & 0x1f);
}

void PPUInterpreter::vexptefp(const Instruction& instr) {
    state.vrs[instr.vd].f[0] = std::pow(2.0f, state.vrs[instr.vb].f[0]);
    state.vrs[instr.vd].f[1] = std::pow(2.0f, state.vrs[instr.vb].f[1]);
    state.vrs[instr.vd].f[2] = std::pow(2.0f, state.vrs[instr.vb].f[2]);
    state.vrs[instr.vd].f[3] = std::pow(2.0f, state.vrs[instr.vb].f[3]);
}

void PPUInterpreter::vmrglw(const Instruction& instr) {
    const VR a = state.vrs[instr.va];
    const VR b = state.vrs[instr.vb];

    state.vrs[instr.vd].w[0] = b.w[0];
    state.vrs[instr.vd].w[1] = a.w[0];
    state.vrs[instr.vd].w[2] = b.w[1];
    state.vrs[instr.vd].w[3] = a.w[1];
}

void PPUInterpreter::vcmpgefp(const Instruction& instr) {
    u8 all_equal = 0x8;
    u8 none_equal = 0x2;

    for (int i = 0; i < 4; i++) {
        if (state.vrs[instr.va].f[i] >= state.vrs[instr.vb].f[i]) {
            state.vrs[instr.vd].w[i] = 0xffffffff;
            none_equal = 0;
        }
        else {
            state.vrs[instr.vd].w[i] = 0;
            all_equal = 0;
        }
    }

    if (instr.rc_v)
        state.cr.setCRField(6, all_equal | none_equal);
}

void PPUInterpreter::vpkswss(const Instruction& instr) {
    u32 src[8];
    for (int i = 0; i < 4; i++) src[i + 0] = state.vrs[instr.vb].w[i];
    for (int i = 0; i < 4; i++) src[i + 4] = state.vrs[instr.va].w[i];
    
    for (int i = 0; i < 8; i++) {
        s32 val = src[i];
        if (val > INT16_MAX)        val = INT16_MAX;
        else if (val < INT16_MIN)   val = INT16_MIN;
        state.vrs[instr.vd].h[i] = val;
    }
}

void PPUInterpreter::vcmpgtub(const Instruction& instr) {
    u8 all_equal = 0x8;
    u8 none_equal = 0x2;

    for (int i = 0; i < 16; i++) {
        if (state.vrs[instr.va].b[i] > state.vrs[instr.vb].b[i]) {
            state.vrs[instr.vd].b[i] = 0xff;
            none_equal = 0;
        }
        else {
            state.vrs[instr.vd].b[i] = 0;
            all_equal = 0;
        }
    }

    if (instr.rc_v)
        state.cr.setCRField(6, all_equal | none_equal);
}

void PPUInterpreter::vspltb(const Instruction& instr) {
    Helpers::debugAssert(instr.uimm < 16, "spltb: uimm >= 16\n");

    const u8 v = state.vrs[instr.vb].b[15 - instr.uimm];

    for (int i = 0; i < 16; i++)
        state.vrs[instr.vd].b[i] = v;
}

void PPUInterpreter::vupkhsb(const Instruction& instr) {
    for (int i = 0; i < 8; i++) {
        state.vrs[instr.vd].h[i] = (s16)(s8)state.vrs[instr.vb].b[8 + i];
    }
}

void PPUInterpreter::vcmpgtuh(const Instruction& instr) {
    u8 all_equal = 0x8;
    u8 none_equal = 0x2;

    for (int i = 0; i < 8; i++) {
        if (state.vrs[instr.va].h[i] > state.vrs[instr.vb].h[i]) {
            state.vrs[instr.vd].h[i] = 0xffff;
            none_equal = 0;
        }
        else {
            state.vrs[instr.vd].h[i] = 0;
            all_equal = 0;
        }
    }

    if (instr.rc_v)
        state.cr.setCRField(6, all_equal | none_equal);
}

void PPUInterpreter::vsplth(const Instruction& instr) {
    Helpers::debugAssert(instr.uimm < 8, "splth: uimm >= 8\n");

    const u16 v = state.vrs[instr.vb].h[7 - instr.uimm];

    state.vrs[instr.vd].h[0] = v;
    state.vrs[instr.vd].h[1] = v;
    state.vrs[instr.vd].h[2] = v;
    state.vrs[instr.vd].h[3] = v;
    state.vrs[instr.vd].h[4] = v;
    state.vrs[instr.vd].h[5] = v;
    state.vrs[instr.vd].h[6] = v;
    state.vrs[instr.vd].h[7] = v;
}

void PPUInterpreter::vupkhsh(const Instruction& instr) {
    state.vrs[instr.vd].w[0] = (s32)(s16)state.vrs[instr.vb].h[4];
    state.vrs[instr.vd].w[1] = (s32)(s16)state.vrs[instr.vb].h[5];
    state.vrs[instr.vd].w[2] = (s32)(s16)state.vrs[instr.vb].h[6];
    state.vrs[instr.vd].w[3] = (s32)(s16)state.vrs[instr.vb].h[7];
}

void PPUInterpreter::vsrw(const Instruction& instr) {
    state.vrs[instr.vd].w[0] = state.vrs[instr.va].w[0] >> (state.vrs[instr.vb].w[0] & 0x1f);
    state.vrs[instr.vd].w[1] = state.vrs[instr.va].w[1] >> (state.vrs[instr.vb].w[1] & 0x1f);
    state.vrs[instr.vd].w[2] = state.vrs[instr.va].w[2] >> (state.vrs[instr.vb].w[2] & 0x1f);
    state.vrs[instr.vd].w[3] = state.vrs[instr.va].w[3] >> (state.vrs[instr.vb].w[3] & 0x1f);
}

void PPUInterpreter::vcmpgtuw(const Instruction& instr) {
    u8 all_equal = 0x8;
    u8 none_equal = 0x2;

    for (int i = 0; i < 4; i++) {
        if (state.vrs[instr.va].w[i] > state.vrs[instr.vb].w[i]) {
            state.vrs[instr.vd].w[i] = 0xffffffff;
            none_equal = 0;
        }
        else {
            state.vrs[instr.vd].w[i] = 0;
            all_equal = 0;
        }
    }

    if (instr.rc_v)
        state.cr.setCRField(6, all_equal | none_equal);
}

void PPUInterpreter::vspltw(const Instruction& instr) {
    Helpers::debugAssert(instr.uimm < 4, "spltw: uimm >= 4\n");

    const u32 v = state.vrs[instr.vb].w[3 - instr.uimm];

    state.vrs[instr.vd].w[0] = v;
    state.vrs[instr.vd].w[1] = v;
    state.vrs[instr.vd].w[2] = v;
    state.vrs[instr.vd].w[3] = v;
}

void PPUInterpreter::vupklsb(const Instruction& instr) {
    for (int i = 0; i < 8; i++) {
        state.vrs[instr.vd].h[i] = (s16)(s8)state.vrs[instr.vb].b[i];
    }
}

void PPUInterpreter::vcmpgtfp(const Instruction& instr) {
    u8 all_equal = 0x8;
    u8 none_equal = 0x2;

    for (int i = 0; i < 4; i++) {
        if (state.vrs[instr.va].f[i] > state.vrs[instr.vb].f[i]) {
            state.vrs[instr.vd].w[i] = 0xffffffff;
            none_equal = 0;
        }
        else {
            state.vrs[instr.vd].w[i] = 0;
            all_equal = 0;
        }
    }

    if (instr.rc_v)
        state.cr.setCRField(6, all_equal | none_equal);
}

void PPUInterpreter::vupklsh(const Instruction& instr) {
    state.vrs[instr.vd].w[0] = (s32)(s16)state.vrs[instr.vb].h[0];
    state.vrs[instr.vd].w[1] = (s32)(s16)state.vrs[instr.vb].h[1];
    state.vrs[instr.vd].w[2] = (s32)(s16)state.vrs[instr.vb].h[2];
    state.vrs[instr.vd].w[3] = (s32)(s16)state.vrs[instr.vb].h[3];
}

void PPUInterpreter::vcfux(const Instruction& instr) {
    const auto factor = 1 << instr.uimm;
    state.vrs[instr.vd].f[0] = (float)state.vrs[instr.vb].w[0] / factor;
    state.vrs[instr.vd].f[1] = (float)state.vrs[instr.vb].w[1] / factor;
    state.vrs[instr.vd].f[2] = (float)state.vrs[instr.vb].w[2] / factor;
    state.vrs[instr.vd].f[3] = (float)state.vrs[instr.vb].w[3] / factor;
}

void PPUInterpreter::vspltisb(const Instruction& instr) {
    s8 si = ((s8)(instr.simm << 3)) >> 3; // Sign extend 5 bit field

    for (int i = 0; i < 16; i++)
        state.vrs[instr.vd].b[i] = si;
}

void PPUInterpreter::vaddshs(const Instruction& instr) {
    for (int i = 0; i < 8; i++) {
        s32 res = (s32)(s16)state.vrs[instr.va].h[i] + (s32)(s16)state.vrs[instr.vb].h[i];
        if (res > 0x7fff) res = 0x7fff;
        else if (res < -0x8000) res = -0x8000;
        state.vrs[instr.vd].h[i] = res;
    }
    // TODO: SAT
}

void PPUInterpreter::vsrah(const Instruction& instr) {
    state.vrs[instr.vd].h[0] = (s16)state.vrs[instr.va].h[0] >> (state.vrs[instr.vb].h[0] & 0xf);
    state.vrs[instr.vd].h[1] = (s16)state.vrs[instr.va].h[1] >> (state.vrs[instr.vb].h[1] & 0xf);
    state.vrs[instr.vd].h[2] = (s16)state.vrs[instr.va].h[2] >> (state.vrs[instr.vb].h[2] & 0xf);
    state.vrs[instr.vd].h[3] = (s16)state.vrs[instr.va].h[3] >> (state.vrs[instr.vb].h[3] & 0xf);
    state.vrs[instr.vd].h[4] = (s16)state.vrs[instr.va].h[4] >> (state.vrs[instr.vb].h[4] & 0xf);
    state.vrs[instr.vd].h[5] = (s16)state.vrs[instr.va].h[5] >> (state.vrs[instr.vb].h[5] & 0xf);
    state.vrs[instr.vd].h[6] = (s16)state.vrs[instr.va].h[6] >> (state.vrs[instr.vb].h[6] & 0xf);
    state.vrs[instr.vd].h[7] = (s16)state.vrs[instr.va].h[7] >> (state.vrs[instr.vb].h[7] & 0xf);
}

void PPUInterpreter::vmulesh(const Instruction& instr) {
    // Even actually means odd for us because of the usual PPC bs
    state.vrs[instr.vd].w[0] = (s32)(s16)state.vrs[instr.va].h[1] * (s32)(s16)state.vrs[instr.vb].h[1];
    state.vrs[instr.vd].w[1] = (s32)(s16)state.vrs[instr.va].h[3] * (s32)(s16)state.vrs[instr.vb].h[3];
    state.vrs[instr.vd].w[2] = (s32)(s16)state.vrs[instr.va].h[5] * (s32)(s16)state.vrs[instr.vb].h[5];
    state.vrs[instr.vd].w[3] = (s32)(s16)state.vrs[instr.va].h[7] * (s32)(s16)state.vrs[instr.vb].h[7];
}

void PPUInterpreter::vcfsx(const Instruction& instr) {
    const auto factor = 1 << instr.uimm;
    state.vrs[instr.vd].f[0] = (float)(s32)state.vrs[instr.vb].w[0] / factor;
    state.vrs[instr.vd].f[1] = (float)(s32)state.vrs[instr.vb].w[1] / factor;
    state.vrs[instr.vd].f[2] = (float)(s32)state.vrs[instr.vb].w[2] / factor;
    state.vrs[instr.vd].f[3] = (float)(s32)state.vrs[instr.vb].w[3] / factor;
}

void PPUInterpreter::vspltish(const Instruction& instr) {
    s16 si = (s16)((s8)(instr.simm << 3)) >> 3; // Sign extend 5 bit field

    state.vrs[instr.vd].h[0] = si;
    state.vrs[instr.vd].h[1] = si;
    state.vrs[instr.vd].h[2] = si;
    state.vrs[instr.vd].h[3] = si;
    state.vrs[instr.vd].h[4] = si;
    state.vrs[instr.vd].h[5] = si;
    state.vrs[instr.vd].h[6] = si;
    state.vrs[instr.vd].h[7] = si;
}

void PPUInterpreter::vsraw(const Instruction& instr) {
    state.vrs[instr.vd].w[0] = (s32)state.vrs[instr.va].w[0] >> (state.vrs[instr.vb].w[0] & 0x1f);
    state.vrs[instr.vd].w[1] = (s32)state.vrs[instr.va].w[1] >> (state.vrs[instr.vb].w[1] & 0x1f);
    state.vrs[instr.vd].w[2] = (s32)state.vrs[instr.va].w[2] >> (state.vrs[instr.vb].w[2] & 0x1f);
    state.vrs[instr.vd].w[3] = (s32)state.vrs[instr.va].w[3] >> (state.vrs[instr.vb].w[3] & 0x1f);
}

void PPUInterpreter::vcmpgtsw(const Instruction& instr) {
    u8 all_equal = 0x8;
    u8 none_equal = 0x2;

    for (int i = 0; i < 4; i++) {
        if ((s32)state.vrs[instr.va].w[i] > (s32)state.vrs[instr.vb].w[i]) {
            state.vrs[instr.vd].w[i] = 0xffffffff;
            none_equal = 0;
        }
        else {
            state.vrs[instr.vd].w[i] = 0;
            all_equal = 0;
        }
    }

    if (instr.rc_v)
        state.cr.setCRField(6, all_equal | none_equal);
}

void PPUInterpreter::vctuxs(const Instruction& instr) {
    const auto factor = 1 << instr.uimm;
    state.vrs[instr.vd].w[0] = (u32)state.vrs[instr.vb].f[0] * factor;
    state.vrs[instr.vd].w[1] = (u32)state.vrs[instr.vb].f[1] * factor;
    state.vrs[instr.vd].w[2] = (u32)state.vrs[instr.vb].f[2] * factor;
    state.vrs[instr.vd].w[3] = (u32)state.vrs[instr.vb].f[3] * factor;

    // TODO: Saturate
}

void PPUInterpreter::vspltisw(const Instruction& instr) {
    s32 si = (s32)((s8)(instr.simm << 3)) >> 3; // Sign extend 5 bit field

    state.vrs[instr.vd].w[0] = si;
    state.vrs[instr.vd].w[1] = si;
    state.vrs[instr.vd].w[2] = si;
    state.vrs[instr.vd].w[3] = si;
}

void PPUInterpreter::vctsxs(const Instruction& instr) {
    const auto factor = 1 << instr.uimm;
    for (int i = 0; i < 4; i++) {
        s64 v = (s64)(state.vrs[instr.vb].f[i] * factor);
        if (v > INT32_MAX) v = INT32_MAX;
        else if (v < INT32_MIN) v = INT32_MIN;
        state.vrs[instr.vd].w[i] = (s32)v;
    }
}

void PPUInterpreter::vand(const Instruction& instr) {
    state.vrs[instr.vd].dw[0] = state.vrs[instr.va].dw[0] & state.vrs[instr.vb].dw[0];
    state.vrs[instr.vd].dw[1] = state.vrs[instr.va].dw[1] & state.vrs[instr.vb].dw[1];
}

void PPUInterpreter::vmaxfp(const Instruction& instr) {
    state.vrs[instr.vd].f[0] = (state.vrs[instr.va].f[0] > state.vrs[instr.vb].f[0]) ? state.vrs[instr.va].f[0] : state.vrs[instr.vb].f[0];
    state.vrs[instr.vd].f[1] = (state.vrs[instr.va].f[1] > state.vrs[instr.vb].f[1]) ? state.vrs[instr.va].f[1] : state.vrs[instr.vb].f[1];
    state.vrs[instr.vd].f[2] = (state.vrs[instr.va].f[2] > state.vrs[instr.vb].f[2]) ? state.vrs[instr.va].f[2] : state.vrs[instr.vb].f[2];
    state.vrs[instr.vd].f[3] = (state.vrs[instr.va].f[3] > state.vrs[instr.vb].f[3]) ? state.vrs[instr.va].f[3] : state.vrs[instr.vb].f[3];
}

void PPUInterpreter::vsubuhm(const Instruction& instr) {
    state.vrs[instr.vd].h[0] = state.vrs[instr.va].h[0] - state.vrs[instr.vb].h[0];
    state.vrs[instr.vd].h[1] = state.vrs[instr.va].h[1] - state.vrs[instr.vb].h[1];
    state.vrs[instr.vd].h[2] = state.vrs[instr.va].h[2] - state.vrs[instr.vb].h[2];
    state.vrs[instr.vd].h[3] = state.vrs[instr.va].h[3] - state.vrs[instr.vb].h[3];
    state.vrs[instr.vd].h[4] = state.vrs[instr.va].h[4] - state.vrs[instr.vb].h[4];
    state.vrs[instr.vd].h[5] = state.vrs[instr.va].h[5] - state.vrs[instr.vb].h[5];
    state.vrs[instr.vd].h[6] = state.vrs[instr.va].h[6] - state.vrs[instr.vb].h[6];
    state.vrs[instr.vd].h[7] = state.vrs[instr.va].h[7] - state.vrs[instr.vb].h[7];
}

void PPUInterpreter::vandc(const Instruction& instr) {
    state.vrs[instr.vd].dw[0] = state.vrs[instr.va].dw[0] & ~state.vrs[instr.vb].dw[0];
    state.vrs[instr.vd].dw[1] = state.vrs[instr.va].dw[1] & ~state.vrs[instr.vb].dw[1];
}

void PPUInterpreter::vminfp(const Instruction& instr) {
    state.vrs[instr.vd].f[0] = (state.vrs[instr.va].f[0] < state.vrs[instr.vb].f[0]) ? state.vrs[instr.va].f[0] : state.vrs[instr.vb].f[0];
    state.vrs[instr.vd].f[1] = (state.vrs[instr.va].f[1] < state.vrs[instr.vb].f[1]) ? state.vrs[instr.va].f[1] : state.vrs[instr.vb].f[1];
    state.vrs[instr.vd].f[2] = (state.vrs[instr.va].f[2] < state.vrs[instr.vb].f[2]) ? state.vrs[instr.va].f[2] : state.vrs[instr.vb].f[2];
    state.vrs[instr.vd].f[3] = (state.vrs[instr.va].f[3] < state.vrs[instr.vb].f[3]) ? state.vrs[instr.va].f[3] : state.vrs[instr.vb].f[3];
}

void PPUInterpreter::vsubuwm(const Instruction& instr) {
    state.vrs[instr.vd].w[0] = state.vrs[instr.va].w[0] - state.vrs[instr.vb].w[0];
    state.vrs[instr.vd].w[1] = state.vrs[instr.va].w[1] - state.vrs[instr.vb].w[1];
    state.vrs[instr.vd].w[2] = state.vrs[instr.va].w[2] - state.vrs[instr.vb].w[2];
    state.vrs[instr.vd].w[3] = state.vrs[instr.va].w[3] - state.vrs[instr.vb].w[3];
}

void PPUInterpreter::vor(const Instruction& instr) {
    state.vrs[instr.vd].dw[0] = state.vrs[instr.va].dw[0] | state.vrs[instr.vb].dw[0];
    state.vrs[instr.vd].dw[1] = state.vrs[instr.va].dw[1] | state.vrs[instr.vb].dw[1];
}

void PPUInterpreter::vnor(const Instruction& instr) {
    state.vrs[instr.vd].dw[0] = ~(state.vrs[instr.va].dw[0] | state.vrs[instr.vb].dw[0]);
    state.vrs[instr.vd].dw[1] = ~(state.vrs[instr.va].dw[1] | state.vrs[instr.vb].dw[1]);
}

void PPUInterpreter::mfvscr(const Instruction& instr) {
    printf("TODO: mfvscr\n");
}

void PPUInterpreter::mtvscr(const Instruction& instr) {
    printf("TODO: mtvscr\n");
}

void PPUInterpreter::vxor(const Instruction& instr) {
    state.vrs[instr.vd].dw[0] = state.vrs[instr.va].dw[0] ^ state.vrs[instr.vb].dw[0];
    state.vrs[instr.vd].dw[1] = state.vrs[instr.va].dw[1] ^ state.vrs[instr.vb].dw[1];
}

void PPUInterpreter::vsum4sbs(const Instruction& instr) {
    for (int i = 0; i < 4; i++) {
        const s64 sum   = (s8)state.vrs[instr.va].b[i * 4 + 0]
                        + (s8)state.vrs[instr.va].b[i * 4 + 1]
                        + (s8)state.vrs[instr.va].b[i * 4 + 2]
                        + (s8)state.vrs[instr.va].b[i * 4 + 3];
        s64 res = sum + (s64)(s32)state.vrs[instr.vb].w[i];
        if (res > INT32_MAX)        res = INT32_MAX;
        else if (res < INT32_MIN)   res = INT32_MIN;
        state.vrs[instr.vd].w[i] = (s32)res;
        // TODO: SAT
    }
}

void PPUInterpreter::vsubshs(const Instruction& instr) {
    for (int i = 0; i < 8; i++) {
        s32 res = (s32)(s16)state.vrs[instr.va].h[i] - (s32)(s16)state.vrs[instr.vb].h[i];
        if (res > 0x7fff) res = 0x7fff;
        else if (res < -0x8000) res = -0x8000;
        state.vrs[instr.vd].h[i] = res;
    }
    // TODO: SAT
}

void PPUInterpreter::vsumsws(const Instruction& instr) {
    s64 res     = (s32)state.vrs[instr.va].w[0]
                + (s32)state.vrs[instr.va].w[1]
                + (s32)state.vrs[instr.va].w[2]
                + (s32)state.vrs[instr.va].w[3]
                + (s32)state.vrs[instr.vb].w[0];
    if (res > INT32_MAX)        res = INT32_MAX;
    else if (res < INT32_MIN)   res = INT32_MIN;
    state.vrs[instr.vd].w[0] = (s32)res;
    state.vrs[instr.vd].w[1] = 0;
    state.vrs[instr.vd].dw[1] = 0;
    // TODO: SAT
}

// G_13

void PPUInterpreter::mcrf(const Instruction& instr) {
    state.cr.setCRField(instr.bf, state.cr.getCRField(instr.bfa));
}

void PPUInterpreter::bclr(const Instruction& instr) {
    auto lr = state.lr;
    if (instr.lk)  // Link
        state.lr = state.pc + 4;

    if (branchCondition(instr.bo, instr.bi)) {
        state.pc = lr;
#ifdef PRINT_DEBUG_SYMBOLS
        printFunctionCall();
#endif
#if !defined(CHONKYSTATION3_USER_BUILD) && defined(TRACK_CALL_STACK)
        if (!call_stack.empty())
            call_stack.pop_back();
#endif
        state.pc -= 4;
    }
}

void PPUInterpreter::crnor(const Instruction& instr) {
    const auto a = (state.cr.raw >> (31 - instr.ba)) & 1;
    const auto b = (state.cr.raw >> (31 - instr.bb)) & 1;
    state.cr.raw &= ~(1 << (31 - instr.bt));
    state.cr.raw |= (~(a | b) & 1) << (31 - instr.bt);
}

void PPUInterpreter::crandc(const Instruction& instr) {
    const auto a = (state.cr.raw >> (31 - instr.ba)) & 1;
    const auto b = (state.cr.raw >> (31 - instr.bb)) & 1;
    state.cr.raw &= ~(1 << (31 - instr.bt));
    state.cr.raw |= ((a & ~b) & 1) << (31 - instr.bt);
}

void PPUInterpreter::crnand(const Instruction& instr) {
    const auto a = (state.cr.raw >> (31 - instr.ba)) & 1;
    const auto b = (state.cr.raw >> (31 - instr.bb)) & 1;
    state.cr.raw &= ~(1 << (31 - instr.bt));
    state.cr.raw |= (~(a & b) & 1) << (31 - instr.bt);
}

void PPUInterpreter::crand(const Instruction& instr) {
    const auto a = (state.cr.raw >> (31 - instr.ba)) & 1;
    const auto b = (state.cr.raw >> (31 - instr.bb)) & 1;
    state.cr.raw &= ~(1 << (31 - instr.bt));
    state.cr.raw |= (a & b) << (31 - instr.bt);
}


void PPUInterpreter::crorc(const Instruction& instr) {
    const auto a = (state.cr.raw >> (31 - instr.ba)) & 1;
    const auto b = (state.cr.raw >> (31 - instr.bb)) & 1;
    state.cr.raw &= ~(1 << (31 - instr.bt));
    state.cr.raw |= ((a | ~b) & 1) << (31 - instr.bt);
}

void PPUInterpreter::cror(const Instruction& instr) {
    const auto a = (state.cr.raw >> (31 - instr.ba)) & 1;
    const auto b = (state.cr.raw >> (31 - instr.bb)) & 1;
    state.cr.raw &= ~(1 << (31 - instr.bt));
    state.cr.raw |= (a | b) << (31 - instr.bt);
}

void PPUInterpreter::bcctr(const Instruction& instr) {
    if (instr.lk)  // Link
        state.lr = state.pc + 4;

    if (branchCondition(instr.bo, instr.bi)) {
        state.pc = state.ctr;
#ifdef PRINT_DEBUG_SYMBOLS
        printFunctionCall();
#endif
#if !defined(CHONKYSTATION3_USER_BUILD) && defined(TRACK_CALL_STACK)
        if (instr.lk)
            call_stack.push_back({ state.pc, state.lr - 4 });
#endif
        state.pc -= 4;
    }
}

// G_1E

void PPUInterpreter::rldicl(const Instruction& instr) {
    const auto sh = instr.sh_lo | (instr.sh_hi << 5);
    const auto mb = ((instr.mb_6 & 1) << 5) | (instr.mb_6 >> 1);
    const auto mask = rotation_mask[mb][63];
    state.gprs[instr.ra] = std::rotl<u64>(state.gprs[instr.rs], sh) & mask;

    if (instr.rc)
        state.cr.compareAndUpdateCRField<s64>(0, state.gprs[instr.ra], 0);
}

void PPUInterpreter::rldicr(const Instruction& instr) {
    const auto sh = instr.sh_lo | (instr.sh_hi << 5);
    const auto me = ((instr.me_6 & 1) << 5) | (instr.me_6 >> 1);
    const auto mask = rotation_mask[0][me];
    state.gprs[instr.ra] = std::rotl<u64>(state.gprs[instr.rs], sh) & mask;
    if (instr.rc)
        state.cr.compareAndUpdateCRField<s64>(0, state.gprs[instr.ra], 0);
}

void PPUInterpreter::rldic(const Instruction& instr) {
    const auto sh = instr.sh_lo | (instr.sh_hi << 5);
    const auto mb = ((instr.mb_6 & 1) << 5) | (instr.mb_6 >> 1);
    const auto mask = rotation_mask[mb][63 - sh];
    state.gprs[instr.ra] = std::rotl<u64>(state.gprs[instr.rs], sh) & mask;
    if (instr.rc)
        state.cr.compareAndUpdateCRField<s64>(0, state.gprs[instr.ra], 0);
}

void PPUInterpreter::rldimi(const Instruction& instr) {
    const auto sh = instr.sh_lo | (instr.sh_hi << 5);
    const auto mb = ((instr.mb_6 & 1) << 5) | (instr.mb_6 >> 1);
    const auto mask = rotation_mask[mb][63 - sh];
    state.gprs[instr.ra] = (state.gprs[instr.ra] & ~mask) | (std::rotl<u64>(state.gprs[instr.rs], sh) & mask);
    if (instr.rc)
        state.cr.compareAndUpdateCRField<s64>(0, state.gprs[instr.ra], 0);
}

void PPUInterpreter::rldcl(const Instruction& instr) {
    const auto sh = state.gprs[instr.rb] & 0x3f;
    const auto mb = ((instr.mb_6 & 1) << 5) | (instr.mb_6 >> 1);
    const auto mask = rotation_mask[mb][63];
    state.gprs[instr.ra] = std::rotl<u64>(state.gprs[instr.rs], sh) & mask;
    if (instr.rc)
        state.cr.compareAndUpdateCRField<s64>(0, state.gprs[instr.ra], 0);
}

// G_1F

void PPUInterpreter::cmp(const Instruction& instr) {
    if (instr.l)
        state.cr.compareAndUpdateCRField<s64>(instr.bf, state.gprs[instr.ra], state.gprs[instr.rb], 0);
    else
        state.cr.compareAndUpdateCRField<s32>(instr.bf, state.gprs[instr.ra], state.gprs[instr.rb], 0);
}

void PPUInterpreter::lvsl(const Instruction& instr) {
    const u32 addr = instr.ra ? (state.gprs[instr.ra] + state.gprs[instr.rb]) : state.gprs[instr.rb];
    state.vrs[instr.vd].dw[0] = lvsl_shifts[addr & 0xf][0];
    state.vrs[instr.vd].dw[1] = lvsl_shifts[addr & 0xf][1];
}

void PPUInterpreter::subfc(const Instruction& instr) {
    Helpers::debugAssert(!instr.oe, "subfc: oe bit set\n");

    const auto a = state.gprs[instr.ra];
    const auto b = state.gprs[instr.rb];
    const auto res1 = ~a + b;
    const auto res2 = res1 + 1;

    // See if ~a + b carried
    const bool carry1 = res1 < b;
    // See if adding 1 carried
    const bool carry2 = res2 < res1;

    state.xer.ca = (carry1 || carry2) ? 1 : 0;
    state.gprs[instr.rt] = res2;

    if (instr.rc)
        state.cr.compareAndUpdateCRField<s64>(0, state.gprs[instr.rt], 0);
}

void PPUInterpreter::mulhdu(const Instruction& instr) {
    const u64 a = state.gprs[instr.ra];
    const u64 b = state.gprs[instr.rb];
#ifdef _MSC_VER
    _umul128(a, b, &state.gprs[instr.rt]);
#else
    using uint128_t = unsigned __int128;

    uint128_t res = (uint128_t)a * b;
    state.gprs[instr.rt] = res >> 64;
#endif
    if (instr.rc)
        state.cr.compareAndUpdateCRField<s64>(0, state.gprs[instr.rt], 0);
}

void PPUInterpreter::addc(const Instruction& instr) {
    Helpers::debugAssert(!instr.oe, "addc: oe bit set\n");
    const auto a = state.gprs[instr.ra];
    const auto b = state.gprs[instr.rb];
    const auto res = a + b;
    state.xer.ca = res < a;
    state.gprs[instr.rt] = res;
    if (instr.rc)
        state.cr.compareAndUpdateCRField<s64>(0, state.gprs[instr.rt], 0);
}

void PPUInterpreter::mulhwu(const Instruction& instr) {
    const u32 a = state.gprs[instr.ra];
    const u32 b = state.gprs[instr.rb];
    state.gprs[instr.rt] = ((u64)a * (u64)b) >> 32;
    if (instr.rc)
        state.cr.compareAndUpdateCRField<s64>(0, state.gprs[instr.rt], 0);
}

void PPUInterpreter::mfcr(const Instruction& instr) {
    if (instr.one) {    // mfocrf
        auto n = 0;
        auto count = 0;
        for (int i = 0; i < 8; i++) {
            if ((instr.fxm >> i) & 1) {
                n = i;
                count++;
            }
        }
        Helpers::debugAssert(count == 1, "mfocrf with count != 1\n");

        //state.gprs[instr.rt] = (u64)state.cr.getCRField(n) << (n * 4);
        state.gprs[instr.rt] = state.cr.raw;
    }
    else {  // mfcrf
        state.gprs[instr.rt] = state.cr.raw;
    }
}

void PPUInterpreter::lwarx(const Instruction& instr) {
    const u32 addr = instr.ra ? (state.gprs[instr.ra] + state.gprs[instr.rb]) : state.gprs[instr.rb];
    // Create reservation
    mem.reserveAddress(addr);
    state.gprs[instr.rt] = mem.read<u32>(addr);
}

void PPUInterpreter::ldx(const Instruction& instr) {
    state.gprs[instr.rt] = mem.read<u64>(instr.ra ? (state.gprs[instr.ra] + state.gprs[instr.rb]) : state.gprs[instr.rb]);
}

void PPUInterpreter::lwzx(const Instruction& instr) {
    state.gprs[instr.rt] = mem.read<u32>(instr.ra ? (state.gprs[instr.ra] + state.gprs[instr.rb]) : state.gprs[instr.rb]);
}

void PPUInterpreter::cntlzw(const Instruction& instr) {
    Helpers::debugAssert(!instr.rc, "cntlzw: rc bit set\n");
    u8 n = 0;
    for (int i = 31; i >= 0; i--) {
        if ((state.gprs[instr.rs] >> i) & 1)
            break;
        else
            n++;
    }
    state.gprs[instr.ra] = n;
    // TODO: what does rc do?
}

void PPUInterpreter::slw(const Instruction& instr) {
    state.gprs[instr.ra] = safeShl<u32>((u32)state.gprs[instr.rs], state.gprs[instr.rb] & 0x3f);
    if (instr.rc)
        state.cr.compareAndUpdateCRField<s32>(0, state.gprs[instr.ra], 0);
}

void PPUInterpreter::sld(const Instruction& instr) {
    state.gprs[instr.ra] = safeShl<u64>(state.gprs[instr.rs], state.gprs[instr.rb] & 0x7f);
    if (instr.rc)
        state.cr.compareAndUpdateCRField<s64>(0, state.gprs[instr.ra], 0);
}

void PPUInterpreter::and_(const Instruction& instr) {
    state.gprs[instr.ra] = state.gprs[instr.rs] & state.gprs[instr.rb];
    if (instr.rc)
        state.cr.compareAndUpdateCRField<s64>(0, state.gprs[instr.ra], 0);
}

void PPUInterpreter::cmpl(const Instruction& instr) {
    if (instr.l)
        state.cr.compareAndUpdateCRField<u64>(instr.bf, state.gprs[instr.ra], state.gprs[instr.rb], 0);
    else
        state.cr.compareAndUpdateCRField<u32>(instr.bf, state.gprs[instr.ra], state.gprs[instr.rb], 0);
}

void PPUInterpreter::lvsr(const Instruction& instr) {
    const u32 addr = instr.ra ? (state.gprs[instr.ra] + state.gprs[instr.rb]) : state.gprs[instr.rb];
    state.vrs[instr.vd].dw[0] = lvsr_shifts[addr & 0xf][0];
    state.vrs[instr.vd].dw[1] = lvsr_shifts[addr & 0xf][1];
}

void PPUInterpreter::subf(const Instruction& instr) {
    Helpers::debugAssert(!instr.oe, "subf: oe bit set\n");

    state.gprs[instr.rt] = state.gprs[instr.rb] - state.gprs[instr.ra];
    if (instr.rc)
        state.cr.compareAndUpdateCRField<s64>(0, state.gprs[instr.rt], 0);
}

void PPUInterpreter::lwzux(const Instruction& instr) {
    const u64 addr = state.gprs[instr.ra] + state.gprs[instr.rb];
    state.gprs[instr.rt] = mem.read<u32>(addr & 0xffffffff);
    state.gprs[instr.ra] = addr;    // Update
}

void PPUInterpreter::cntlzd(const Instruction& instr) {
    Helpers::debugAssert(!instr.rc, "cntlzd: rc bit set\n");
    u8 n = 0;
    for (int i = 63; i >= 0; i--) {
        if ((state.gprs[instr.rs] >> i) & 1)
            break;
        else
            n++;
    }
    state.gprs[instr.ra] = n;
    // TODO: what does rc do?
}


void PPUInterpreter::andc(const Instruction& instr) {
    state.gprs[instr.ra] = state.gprs[instr.rs] & ~state.gprs[instr.rb];
    if (instr.rc)
        state.cr.compareAndUpdateCRField<s64>(0, state.gprs[instr.ra], 0);
}

void PPUInterpreter::lvewx(const Instruction& instr) {
    const u32 addr = (instr.ra ? (state.gprs[instr.ra] + state.gprs[instr.rb]) : state.gprs[instr.rb]) & ~3;
    const u8 m = (addr >> 2) & 3;
    state.vrs[instr.vd].w[3 - m] = ps3->mem.read<u32>(addr);
}

void PPUInterpreter::mulhd(const Instruction& instr) {
    const s64 a = state.gprs[instr.ra];
    const s64 b = state.gprs[instr.rb];
#ifdef _MSC_VER
    _mul128(a, b, (s64*)&state.gprs[instr.rt]);
#else
    using int128_t = __int128;

    int128_t res = (int128_t)a * b;
    state.gprs[instr.rt] = res >> 64;
#endif
    if (instr.rc)
        state.cr.compareAndUpdateCRField<s64>(0, state.gprs[instr.rt], 0);
}

void PPUInterpreter::mulhw(const Instruction& instr) {
    const s32 a = state.gprs[instr.ra];
    const s32 b = state.gprs[instr.rb];
    state.gprs[instr.rt] = ((s64)a * (s64)b) >> 32;
    if (instr.rc)
        state.cr.compareAndUpdateCRField<s64>(0, state.gprs[instr.rt], 0);
}

void PPUInterpreter::ldarx(const Instruction& instr) {
    const u32 addr = instr.ra ? (state.gprs[instr.ra] + state.gprs[instr.rb]) : state.gprs[instr.rb];
    // Create reservation
    mem.reserveAddress(addr);
    state.gprs[instr.rt] = mem.read<u64>(addr);
}

void PPUInterpreter::lbzx(const Instruction& instr) {
    state.gprs[instr.rt] = mem.read<u8>(instr.ra ? (state.gprs[instr.ra] + state.gprs[instr.rb]) : state.gprs[instr.rb]);
}

void PPUInterpreter::lvx(const Instruction& instr) {
    const u32 addr = (instr.ra ? (state.gprs[instr.ra] + state.gprs[instr.rb]) : state.gprs[instr.rb]) & ~0xf;
    state.vrs[instr.vd].dw[1] = mem.read<u64>(addr);
    state.vrs[instr.vd].dw[0] = mem.read<u64>(addr + 8);
}

void PPUInterpreter::neg(const Instruction& instr) {
    Helpers::debugAssert(!instr.oe, "neg: oe bit set\n");

    state.gprs[instr.rt] = -state.gprs[instr.ra];
    if (instr.rc)
        state.cr.compareAndUpdateCRField<s64>(0, state.gprs[instr.rt], 0);
}

void PPUInterpreter::nor(const Instruction& instr) {
    state.gprs[instr.ra] = ~(state.gprs[instr.rs] | state.gprs[instr.rb]);
    if (instr.rc)
        state.cr.compareAndUpdateCRField<s64>(0, state.gprs[instr.ra], 0);
}

void PPUInterpreter::stvebx(const Instruction& instr) {
    const u32 addr = instr.ra ? (state.gprs[instr.ra] + state.gprs[instr.rb]) : state.gprs[instr.rb];
    const u8 m = addr & 0xf;
    ps3->mem.write<u8>(addr, state.vrs[instr.vs].b[15 - m]);
}

void PPUInterpreter::subfe(const Instruction& instr) {
    Helpers::debugAssert(!instr.oe, "subfe: oe bit set\n");

    const auto a = state.gprs[instr.ra];
    const auto b = state.gprs[instr.rb];
    const auto res1 = ~a + b;
    const auto res2 = res1 + state.xer.ca;

    // See if ~a + b carried
    const bool carry1 = res1 < b;
    // See if adding carry carried
    const bool carry2 = res2 < res1;

    state.xer.ca = (carry1 || carry2) ? 1 : 0;
    state.gprs[instr.rt] = res2;

    if (instr.rc)
        state.cr.compareAndUpdateCRField<s64>(0, state.gprs[instr.rt], 0);
}

void PPUInterpreter::adde(const Instruction& instr) {
    Helpers::debugAssert(!instr.oe, "adde: oe bit set\n");

    const auto a = state.gprs[instr.ra];
    const auto b = state.gprs[instr.rb];
    const auto res1 = a + b;
    const auto res2 = res1 + state.xer.ca;

    // See if a + b carried
    const bool carry1 = res1 < b;
    // See if adding carry carried
    const bool carry2 = res2 < res1;

    state.xer.ca = (carry1 || carry2) ? 1 : 0;
    state.gprs[instr.rt] = res2;

    if (instr.rc)
        state.cr.compareAndUpdateCRField<s64>(0, state.gprs[instr.rt], 0);
}

void PPUInterpreter::mtcrf(const Instruction& instr) {
    if (instr.one) {    // mtocrf
        auto n = 0;
        auto count = 0;
        for (int i = 0; i < 8; i++) {
            if ((instr.fxm >> i) & 1) {
                n = i;
                count++;
            }
        }

        Helpers::debugAssert(count == 1, "mtocrf with count != 1\n");
        state.cr.setCRField(7 - n, (state.gprs[instr.rs] >> (4 * n)) & 0xf);
    }
    else {  // mtcrf
        for (int i = 0; i < 8; i++) {
            if ((instr.fxm >> i) & 1) {
                state.cr.setCRField(7 - i, (state.gprs[instr.rs] >> (4 * i)) & 0xf);
            }
        }
    }
}

void PPUInterpreter::stdx(const Instruction& instr) {
    mem.write<u64>(instr.ra ? (state.gprs[instr.ra] + state.gprs[instr.rb]) : state.gprs[instr.rb], state.gprs[instr.rs]);
}

void PPUInterpreter::stwcx(const Instruction& instr) {
    const u32 addr = instr.ra ? (state.gprs[instr.ra] + state.gprs[instr.rb]) : state.gprs[instr.rb];
    // Try to acquire reservation
    bool success = mem.acquireReservation(addr);
    // Conditionally write
    if (success)
        mem.write<u32>(addr, state.gprs[instr.rs]);
    // Update CR
    state.cr.setCRField(0, success ? ConditionRegister::EQUAL : 0);
}

void PPUInterpreter::stwx(const Instruction& instr) {
    mem.write<u32>(instr.ra ? (state.gprs[instr.ra] + state.gprs[instr.rb]) : state.gprs[instr.rb], state.gprs[instr.rs]);
}

void PPUInterpreter::stvehx(const Instruction& instr) {
    const u32 addr = (instr.ra ? (state.gprs[instr.ra] + state.gprs[instr.rb]) : state.gprs[instr.rb]) & ~1;
    const u8 m = (addr & 0xf) >> 1;
    ps3->mem.write<u16>(addr, state.vrs[instr.vs].h[7 - m]);
}

void PPUInterpreter::stdux(const Instruction& instr) {
    const u64 addr = state.gprs[instr.ra] + state.gprs[instr.rb];
    mem.write<u64>(addr & 0xffffffff, state.gprs[instr.rs]);
    state.gprs[instr.ra] = addr;    // Update
}

void PPUInterpreter::stvewx(const Instruction& instr) {
    const u32 addr = (instr.ra ? (state.gprs[instr.ra] + state.gprs[instr.rb]) : state.gprs[instr.rb]) & ~3;
    const u8 m = (addr & 0xf) >> 2;
    ps3->mem.write<u32>(addr, state.vrs[instr.vs].w[3 - m]);
}

void PPUInterpreter::addze(const Instruction& instr) {
    Helpers::debugAssert(!instr.oe, "add: oe bit set\n");

    const auto a = state.gprs[instr.ra];
    const u8 b = state.xer.ca;
    const auto res = a + b;
    state.gprs[instr.rt] = res;
    state.xer.ca = res < a;

    if (instr.rc)
        state.cr.compareAndUpdateCRField<s64>(0, state.gprs[instr.rt], 0);
}

void PPUInterpreter::stdcx(const Instruction& instr) {
    const u32 addr = instr.ra ? (state.gprs[instr.ra] + state.gprs[instr.rb]) : state.gprs[instr.rb];
    // Try to acquire reservation
    bool success = mem.acquireReservation(addr);
    // Conditionally write
    if (success)
        mem.write<u64>(addr, state.gprs[instr.rs]);
    // Update CR
    state.cr.setCRField(0, success ? ConditionRegister::EQUAL : 0);
}

void PPUInterpreter::stbx(const Instruction& instr) {
    mem.write<u8>(instr.ra ? (state.gprs[instr.ra] + state.gprs[instr.rb]) : state.gprs[instr.rb], state.gprs[instr.rs]);
}

void PPUInterpreter::stvx(const Instruction& instr) {
    const u32 addr = (instr.ra ? (state.gprs[instr.ra] + state.gprs[instr.rb]) : state.gprs[instr.rb]) & ~0xf;
    mem.write<u64>(addr, state.vrs[instr.vs].dw[1]);
    mem.write<u64>(addr + 8, state.vrs[instr.vs].dw[0]);
}

void PPUInterpreter::mulld(const Instruction& instr) {
    Helpers::debugAssert(!instr.oe, "mulld: oe bit set\n");

    state.gprs[instr.rt] = (s64)state.gprs[instr.ra] * (s64)state.gprs[instr.rb];
    if (instr.rc)
        state.cr.compareAndUpdateCRField<s64>(0, state.gprs[instr.rt], 0);
}

void PPUInterpreter::mullw(const Instruction& instr) {
    Helpers::debugAssert(!instr.oe, "mullw: oe bit set\n");

    state.gprs[instr.rt] = (s64)((s64)(s32)state.gprs[instr.ra] * (s64)(s32)state.gprs[instr.rb]);
    if (instr.rc)
        state.cr.compareAndUpdateCRField<s64>(0, state.gprs[instr.rt], 0);
}

void PPUInterpreter::stbux(const Instruction& instr) {
    const u64 addr = state.gprs[instr.ra] + state.gprs[instr.rb];
    mem.write<u8>(addr & 0xffffffff, state.gprs[instr.rs]);
    state.gprs[instr.ra] = addr;    // Update
}

void PPUInterpreter::add(const Instruction& instr) {
    Helpers::debugAssert(!instr.oe, "add: oe bit set\n");

    state.gprs[instr.rt] = state.gprs[instr.ra] + state.gprs[instr.rb];
    if (instr.rc)
        state.cr.compareAndUpdateCRField<s64>(0, state.gprs[instr.rt], 0);
}

void PPUInterpreter::lhzx(const Instruction& instr) {
    state.gprs[instr.rt] = mem.read<u16>(instr.ra ? (state.gprs[instr.ra] + state.gprs[instr.rb]) : state.gprs[instr.rb]);
}

void PPUInterpreter::lhzux(const Instruction& instr) {
    const u64 addr = state.gprs[instr.ra] + state.gprs[instr.rb];
    state.gprs[instr.rt] = mem.read<u16>(addr & 0xffffffff);
    state.gprs[instr.ra] = addr;    // Update
}

void PPUInterpreter::xor_(const Instruction& instr) {
    state.gprs[instr.ra] = state.gprs[instr.rs] ^ state.gprs[instr.rb];
    if (instr.rc)
        state.cr.compareAndUpdateCRField<s64>(0, state.gprs[instr.ra], 0);
}

void PPUInterpreter::mfspr(const Instruction& instr) {
    auto reversed_spr = ((instr.spr & 0x1f) << 5) | (instr.spr >> 5);
    switch (reversed_spr) {
    case 0b01000:   state.gprs[instr.rt] = state.lr;        break;
    case 0b01001:   state.gprs[instr.rt] = state.ctr;       break;
    case 0x001:     state.gprs[instr.rt] = state.xer.get(); break;
    case 0x100:     state.gprs[instr.rt] = state.vrsave;    break;
    default: Helpers::panic("mfspr: unimplemented register 0x%04x\n", reversed_spr);
    }
}

void PPUInterpreter::mftb(const Instruction& instr) {
    state.gprs[instr.rt] = std::chrono::system_clock::now().time_since_epoch().count() * 8;
}

void PPUInterpreter::sthx(const Instruction& instr) {
    mem.write<u16>(instr.ra ? (state.gprs[instr.ra] + state.gprs[instr.rb]) : state.gprs[instr.rb], state.gprs[instr.rs]);
}

void PPUInterpreter::orc(const Instruction& instr) {
    state.gprs[instr.ra] = state.gprs[instr.rs] | ~state.gprs[instr.rb];
    if (instr.rc)
        state.cr.compareAndUpdateCRField<s64>(0, state.gprs[instr.ra], 0);
}

void PPUInterpreter::or_(const Instruction& instr) {
    state.gprs[instr.ra] = state.gprs[instr.rs] | state.gprs[instr.rb];
    if (instr.rc)
        state.cr.compareAndUpdateCRField<s64>(0, state.gprs[instr.ra], 0);
}

void PPUInterpreter::divdu(const Instruction& instr) {
    const auto a = state.gprs[instr.ra];
    const auto b = state.gprs[instr.rb];
    Helpers::debugAssert(!instr.oe, "divdu: oe bit set\n");

    if (b == 0) {
        state.gprs[instr.rt] = 0;
        return;
    }

    state.gprs[instr.rt] = a / b;
    if (instr.rc)
        state.cr.compareAndUpdateCRField<s64>(0, state.gprs[instr.rt], 0);
}

void PPUInterpreter::divwu(const Instruction& instr) {
    const u32 a = state.gprs[instr.ra];
    const u32 b = state.gprs[instr.rb];
    Helpers::debugAssert(!instr.oe, "divwu: oe bit set\n");

    if (b == 0) {
        state.gprs[instr.rt] = 0;
        return;
    }

    state.gprs[instr.rt] = a / b;
    if (instr.rc)
        state.cr.compareAndUpdateCRField<s64>(0, state.gprs[instr.rt], 0);
}

void PPUInterpreter::mtspr(const Instruction& instr) {
    auto reversed_spr = ((instr.spr & 0x1f) << 5) | (instr.spr >> 5);
    switch (reversed_spr) {
    case 0b01000:   state.lr     = state.gprs[instr.rs];    break;
    case 0b01001:   state.ctr    = state.gprs[instr.rs];    break;
    case 0x100:     state.vrsave = state.gprs[instr.rs];    break;
    default: Helpers::panic("mtspr: unimplemented register 0x%04x\n", reversed_spr);
    }
}

void PPUInterpreter::nand(const Instruction& instr) {
    state.gprs[instr.ra] = ~(state.gprs[instr.rs] & state.gprs[instr.rb]);
    if (instr.rc)
        state.cr.compareAndUpdateCRField<s64>(0, state.gprs[instr.ra], 0);
}

void PPUInterpreter::divd(const Instruction& instr) {
    const s64 a = state.gprs[instr.ra];
    const s64 b = state.gprs[instr.rb];
    Helpers::debugAssert(!instr.oe, "divd: oe bit set\n");

    if (b == 0 || (a == 0x8000000000000000 && b == -1)) {
        state.gprs[instr.rt] = 0;
        return;
    }

    state.gprs[instr.rt] = a / b;
    if (instr.rc)
        state.cr.compareAndUpdateCRField<s64>(0, state.gprs[instr.rt], 0);
}

void PPUInterpreter::divw(const Instruction& instr) {
    const s32 a = state.gprs[instr.ra];
    const s32 b = state.gprs[instr.rb];
    Helpers::debugAssert(!instr.oe, "divw: oe bit set\n");

    if (b == 0 || (a == 0x80000000 && b == -1)) {
        // This happens in Doom Classic
        state.gprs[instr.rt] = 0;
        return;
    }
    
    state.gprs[instr.rt] = (u32)(a / b);
    if (instr.rc)
        state.cr.compareAndUpdateCRField<s64>(0, state.gprs[instr.rt], 0);
}

void PPUInterpreter::lvlx(const Instruction& instr) {
    const u32 addr = instr.ra ? (state.gprs[instr.ra] + state.gprs[instr.rb]) : state.gprs[instr.rb];
    const u32 b = addr & 0xf;

    state.vrs[instr.vd].dw[0] = 0;
    state.vrs[instr.vd].dw[1] = 0;
    for (int i = 0; i < 16 - b; i++)
        state.vrs[instr.vd].b[15 - i] = ps3->mem.read<u8>(addr + i);
}

void PPUInterpreter::lwbrx(const Instruction& instr) {
    state.gprs[instr.rt] = *(u32*)ps3->mem.getPtr(instr.ra ? (state.gprs[instr.ra] + state.gprs[instr.rb]) : state.gprs[instr.rb]);
}

void PPUInterpreter::lfsx(const Instruction& instr) {
    u32 v = mem.read<u32>(instr.ra ? (state.gprs[instr.ra] + state.gprs[instr.rb]) : state.gprs[instr.rb]);
    state.fprs[instr.frt] = reinterpret_cast<float&>(v);
}

void PPUInterpreter::srw(const Instruction& instr) {
    state.gprs[instr.ra] = safeShr<u32>((u32)state.gprs[instr.rs], state.gprs[instr.rb] & 0x3f);
    if (instr.rc)
        state.cr.compareAndUpdateCRField<s64>(0, state.gprs[instr.ra], 0);
}

void PPUInterpreter::srd(const Instruction& instr) {
    state.gprs[instr.ra] = safeShr<u64>(state.gprs[instr.rs], state.gprs[instr.rb] & 0x7f);
    if (instr.rc)
        state.cr.compareAndUpdateCRField<s64>(0, state.gprs[instr.ra], 0);
}

void PPUInterpreter::lvrx(const Instruction& instr) {
    const u32 addr = instr.ra ? (state.gprs[instr.ra] + state.gprs[instr.rb]) : state.gprs[instr.rb];
    const u32 b = addr & 0xf;

    state.vrs[instr.vd].dw[0] = 0;
    state.vrs[instr.vd].dw[1] = 0;
    for (int i = 16 - b; i < 16; i++)
        state.vrs[instr.vd].b[15 - i] = ps3->mem.read<u8>(addr + i - 16);
}

void PPUInterpreter::lfdx(const Instruction& instr) {
    u64 v = mem.read<u64>(instr.ra ? (state.gprs[instr.ra] + state.gprs[instr.rb]) : state.gprs[instr.rb]);
    state.fprs[instr.frt] = reinterpret_cast<double&>(v);
}

void PPUInterpreter::stvlx(const Instruction& instr) {
    const u32 addr = instr.ra ? (state.gprs[instr.ra] + state.gprs[instr.rb]) : state.gprs[instr.rb];
    const u32 b = addr & 0xf;

    for (int i = 0; i < 16 - b; i++)
        ps3->mem.write<u8>(addr + i, state.vrs[instr.vs].b[15 - i]);
}

void PPUInterpreter::stfsx(const Instruction& instr) {
    const u32 addr = instr.ra ? (state.gprs[instr.ra] + state.gprs[instr.rb]) : state.gprs[instr.rb];
    float v = (float)state.fprs[instr.frs];
    ps3->mem.write<u32>(addr, reinterpret_cast<u32&>(v));
}

void PPUInterpreter::stvrx(const Instruction& instr) {
    const u32 addr = instr.ra ? (state.gprs[instr.ra] + state.gprs[instr.rb]) : state.gprs[instr.rb];
    const u32 b = addr & 0xf;

    for (int i = 16 - b; i < 16; i++)
        ps3->mem.write<u8>(addr + i - 16, state.vrs[instr.vs].b[15 - i]);
}

void PPUInterpreter::stfdx(const Instruction& instr) {
    const u32 addr = instr.ra ? (state.gprs[instr.ra] + state.gprs[instr.rb]) : state.gprs[instr.rb];
    ps3->mem.write<u64>(addr, reinterpret_cast<u64&>(state.fprs[instr.frs]));
}

void PPUInterpreter::lhbrx(const Instruction& instr) {
    state.gprs[instr.rt] = *(u16*)ps3->mem.getPtr(instr.ra ? (state.gprs[instr.ra] + state.gprs[instr.rb]) : state.gprs[instr.rb]);
}

void PPUInterpreter::sraw(const Instruction& instr) {
    const u32 sh = state.gprs[instr.rb] & 0x3f;
    //Helpers::debugAssert(shift <= 32, "sraw: shift > 31");
    // TODO: XER
    if (sh > 31)
        state.gprs[instr.ra] = ((s32)state.gprs[instr.rs] < 0) ? -1 : 0;
    else
        state.gprs[instr.ra] = (s32)state.gprs[instr.rs] >> sh;
    if (instr.rc)
        state.cr.compareAndUpdateCRField<s64>(0, state.gprs[instr.ra], 0);
}

void PPUInterpreter::srad(const Instruction& instr) {
    const u32 sh = state.gprs[instr.rb] & 0x7f;
    //Helpers::debugAssert(shift <= 32, "sraw: shift > 31");
    // TODO: XER
    if (sh > 63)
        state.gprs[instr.ra] = ((s64)state.gprs[instr.rs] < 0) ? -1 : 0;
    else
        state.gprs[instr.ra] = (s64)state.gprs[instr.rs] >> sh;
    if (instr.rc)
        state.cr.compareAndUpdateCRField<s64>(0, state.gprs[instr.ra], 0);
}

void PPUInterpreter::srawi(const Instruction& instr) {
    const auto sh = instr.sh_lo;
    state.gprs[instr.ra] = (s32)state.gprs[instr.rs] >> sh;
    state.xer.ca = ((s32)state.gprs[instr.rs] < 0) && (((u32)state.gprs[instr.ra] << sh) != (u32)state.gprs[instr.rs]);
    if (instr.rc)
        state.cr.compareAndUpdateCRField<s64>(0, state.gprs[instr.ra], 0);
}

void PPUInterpreter::sradi(const Instruction& instr) {
    const auto sh = instr.sh_lo | (instr.sh_hi << 5);
    state.gprs[instr.ra] = (s64)state.gprs[instr.rs] >> sh;
    state.xer.ca = ((s64)state.gprs[instr.rs] < 0) && ((state.gprs[instr.ra] << sh) != state.gprs[instr.rs]);
    if (instr.rc)
        state.cr.compareAndUpdateCRField<s64>(0, state.gprs[instr.ra], 0);
}

void PPUInterpreter::extsh(const Instruction& instr) {
    state.gprs[instr.ra] = (s64)(s16)state.gprs[instr.rs];
    if (instr.rc)
        state.cr.compareAndUpdateCRField<s64>(0, state.gprs[instr.ra], 0);
}

void PPUInterpreter::extsb(const Instruction& instr) {
    state.gprs[instr.ra] = (s64)(s8)state.gprs[instr.rs];
    if (instr.rc)
        state.cr.compareAndUpdateCRField<s64>(0, state.gprs[instr.ra], 0);
}

void PPUInterpreter::extsw(const Instruction& instr) {
    state.gprs[instr.ra] = (s64)(s32)state.gprs[instr.rs];
    if (instr.rc)
        state.cr.compareAndUpdateCRField<s64>(0, state.gprs[instr.ra], 0);
}

void PPUInterpreter::stfiwx(const Instruction& instr) {
    mem.write<u32>(instr.ra ? (state.gprs[instr.ra] + state.gprs[instr.rb]) : state.gprs[instr.rb], reinterpret_cast<u32&>(state.fprs[instr.frs]));
}

void PPUInterpreter::dcbz(const Instruction& instr) {
    const u32 addr = instr.ra ? (state.gprs[instr.ra] + state.gprs[instr.rb]) : state.gprs[instr.rb];
    std::memset(ps3->mem.getPtr(addr & ~127), 0, 128);
}

// G_3A

void PPUInterpreter::ld(const Instruction& instr) {
    const s32 sds = (s32)(s16)(instr.ds << 2);
    const u32 addr = (instr.ra == 0) ? sds : state.gprs[instr.ra] + sds;
    state.gprs[instr.rt] = mem.read<u64>(addr);
}

void PPUInterpreter::ldu(const Instruction& instr) {
    const s32 sds = (s32)(s16)(instr.ds << 2);
    const u64 addr = state.gprs[instr.ra] + sds;
    state.gprs[instr.rt] = mem.read<u64>(addr & 0xffffffff);
    state.gprs[instr.ra] = addr;    // Update
}

void PPUInterpreter::lwa(const Instruction& instr) {
    const s32 sds = (s32)(s16)(instr.ds << 2);
    const u32 addr = (instr.ra == 0) ? sds : state.gprs[instr.ra] + sds;
    state.gprs[instr.rt] = (s64)(s32)mem.read<u32>(addr);
}

// G_3B

void PPUInterpreter::fdivs(const Instruction& instr) {
    // TODO: division by 0? NaNs? inf?
    Helpers::debugAssert(!instr.rc, "fdivs: rc\n");
    state.fprs[instr.frt] = (float)(state.fprs[instr.fra] / state.fprs[instr.frb]);
}

void PPUInterpreter::fsubs(const Instruction& instr) {
    Helpers::debugAssert(!instr.rc, "fsubs: rc\n");
    state.fprs[instr.frt] = (float)(state.fprs[instr.fra] - state.fprs[instr.frb]);
}

void PPUInterpreter::fadds(const Instruction& instr) {
    Helpers::debugAssert(!instr.rc, "fadds: rc\n");
    state.fprs[instr.frt] = (float)(state.fprs[instr.fra] + state.fprs[instr.frb]);
}

void PPUInterpreter::fsqrts(const Instruction& instr) {
    Helpers::debugAssert(!instr.rc, "fadds: rc\n");
    state.fprs[instr.frt] = (float)(sqrtf(state.fprs[instr.frb]));
}

void PPUInterpreter::fmuls(const Instruction& instr) {
    Helpers::debugAssert(!instr.rc, "fmuls: rc\n");
    state.fprs[instr.frt] = (float)(state.fprs[instr.fra] * state.fprs[instr.frc]);
}

void PPUInterpreter::fmsubs(const Instruction& instr) {
    Helpers::debugAssert(!instr.rc, "fmsubs: rc\n");
    state.fprs[instr.frt] = (float)(state.fprs[instr.fra] * state.fprs[instr.frc] - state.fprs[instr.frb]);
}

void PPUInterpreter::fmadds(const Instruction& instr) {
    Helpers::debugAssert(!instr.rc, "fmadds: rc\n");
    state.fprs[instr.frt] = (float)(state.fprs[instr.fra] * state.fprs[instr.frc] + state.fprs[instr.frb]);
}

void PPUInterpreter::fnmsubs(const Instruction& instr) {
    Helpers::debugAssert(!instr.rc, "fnmsubs: rc\n");
    state.fprs[instr.frt] = (float)(-(state.fprs[instr.fra] * state.fprs[instr.frc] - state.fprs[instr.frb]));
}

void PPUInterpreter::fnmadds(const Instruction& instr) {
    Helpers::debugAssert(!instr.rc, "fnmadds: rc\n");
    state.fprs[instr.frt] = (float)(-(state.fprs[instr.fra] * state.fprs[instr.frc] + state.fprs[instr.frb]));
}

// G_3E

void PPUInterpreter::std(const Instruction& instr) {
    const s32 sds = (s32)(s16)(instr.ds << 2);
    const u32 addr = (instr.ra == 0) ? sds : state.gprs[instr.ra] + sds;
    mem.write<u64>(addr, state.gprs[instr.rs]);
}

void PPUInterpreter::stdu(const Instruction& instr) {
    const s32 sds = (s32)(s16)(instr.ds << 2);
    const u64 addr = state.gprs[instr.ra] + sds;
    mem.write<u64>(addr & 0xffffffff, state.gprs[instr.rs]);
    state.gprs[instr.ra] = addr;    // Update
}

// G_3F

void PPUInterpreter::mffs(const Instruction& instr) {
    // TODO
    state.fprs[instr.frt] = 0;
}

void PPUInterpreter::mtfsf(const Instruction& instr) {
    // TODO
}

void PPUInterpreter::fcmpu(const Instruction& instr) {
    state.cr.compareAndUpdateCRField<double>(instr.bf, state.fprs[instr.fra], state.fprs[instr.frb], 0);
}

void PPUInterpreter::frsp(const Instruction& instr) {
    Helpers::debugAssert(!instr.rc, "frsp: rc\n");
    state.fprs[instr.frt] = (float)state.fprs[instr.frb];
}

void PPUInterpreter::fctiwz(const Instruction& instr) {
    Helpers::debugAssert(!instr.rc, "fctiwz: rc\n");
    s64 v = (s64)state.fprs[instr.frb];
    if (v > INT32_MAX) v = INT32_MAX;
    else if (v < INT32_MIN) v = INT32_MIN;
    reinterpret_cast<u64&>(state.fprs[instr.frt]) = 0xfff8000000000000ull | v;
}

void PPUInterpreter::fdiv(const Instruction& instr) {
    // TODO: division by 0? NaNs? inf?
    Helpers::debugAssert(!instr.rc, "fdiv: rc\n");
    state.fprs[instr.frt] = state.fprs[instr.fra] / state.fprs[instr.frb];
}

void PPUInterpreter::fsub(const Instruction& instr) {
    Helpers::debugAssert(!instr.rc, "fsub: rc\n");
    state.fprs[instr.frt] = state.fprs[instr.fra] - state.fprs[instr.frb];
}

void PPUInterpreter::fadd(const Instruction& instr) {
    Helpers::debugAssert(!instr.rc, "fadd: rc\n");
    state.fprs[instr.frt] = state.fprs[instr.fra] + state.fprs[instr.frb];
}

void PPUInterpreter::fsqrt(const Instruction& instr) {
    Helpers::debugAssert(!instr.rc, "fsqrt: rc\n");
    state.fprs[instr.frt] = sqrt(state.fprs[instr.frb]);
}

void PPUInterpreter::fmr(const Instruction& instr) {
    Helpers::debugAssert(!instr.rc, "fmr: rc\n");
    state.fprs[instr.frt] = state.fprs[instr.frb];
}

void PPUInterpreter::fsel(const Instruction& instr) {
    Helpers::debugAssert(!instr.rc, "fsel: rc\n");
    state.fprs[instr.frt] = (state.fprs[instr.fra] >= 0.0) ? state.fprs[instr.frc] : state.fprs[instr.frb];
}

void PPUInterpreter::fmul(const Instruction& instr) {
    Helpers::debugAssert(!instr.rc, "fmul: rc\n");
    state.fprs[instr.frt] = state.fprs[instr.fra] * state.fprs[instr.frc];
}

void PPUInterpreter::frsqrte(const Instruction& instr) {
    Helpers::debugAssert(!instr.rc, "frsqrte: rc\n");
    state.fprs[instr.frt] = 1.0 / sqrt(state.fprs[instr.frb]);
}

void PPUInterpreter::fmsub(const Instruction& instr) {
    Helpers::debugAssert(!instr.rc, "fmsub: rc\n");
    state.fprs[instr.frt] = state.fprs[instr.fra] * state.fprs[instr.frc] - state.fprs[instr.frb];
}

void PPUInterpreter::fmadd(const Instruction& instr) {
    Helpers::debugAssert(!instr.rc, "fmadd: rc\n");
    state.fprs[instr.frt] = state.fprs[instr.fra] * state.fprs[instr.frc] + state.fprs[instr.frb];
}

void PPUInterpreter::fnmsub(const Instruction& instr) {
    Helpers::debugAssert(!instr.rc, "fnmsub: rc\n");
    state.fprs[instr.frt] = -(state.fprs[instr.fra] * state.fprs[instr.frc] - state.fprs[instr.frb]);
}

void PPUInterpreter::fnmadd(const Instruction& instr) {
    Helpers::debugAssert(!instr.rc, "fnmadd: rc\n");
    state.fprs[instr.frt] = -(state.fprs[instr.fra] * state.fprs[instr.frc] + state.fprs[instr.frb]);
}

void PPUInterpreter::fneg(const Instruction& instr) {
    Helpers::debugAssert(!instr.rc, "fneg: rc\n");
    state.fprs[instr.frt] = -state.fprs[instr.frb];
}

void PPUInterpreter::fnabs(const Instruction& instr) {
    Helpers::debugAssert(!instr.rc, "fnabs: rc\n");
    state.fprs[instr.frt] = -fabs(state.fprs[instr.frb]);
}

void PPUInterpreter::fabs_(const Instruction& instr) {
    Helpers::debugAssert(!instr.rc, "fabs: rc\n");
    state.fprs[instr.frt] = fabs(state.fprs[instr.frb]);
}

void PPUInterpreter::fctid(const Instruction& instr) {
    Helpers::debugAssert(!instr.rc, "fctid: rc\n");
    reinterpret_cast<s64&>(state.fprs[instr.frt]) = (s64)state.fprs[instr.frb];
}

void PPUInterpreter::fctidz(const Instruction& instr) {
    Helpers::debugAssert(!instr.rc, "fctidz: rc\n");
    reinterpret_cast<s64&>(state.fprs[instr.frt]) = (s64)state.fprs[instr.frb];
}

void PPUInterpreter::fcfid(const Instruction& instr) {
    Helpers::debugAssert(!instr.rc, "fcfid: rc\n");
    state.fprs[instr.frt] = (double)reinterpret_cast<s64&>(state.fprs[instr.frb]);
}
