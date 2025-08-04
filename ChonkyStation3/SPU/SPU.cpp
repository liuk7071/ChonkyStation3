#include "SPU.hpp"


int SPU::step() {
    Helpers::panic("Backend did not define step function\n");
}

void SPU::clr(SPUTypes::GPR& gpr) {
    gpr.dw[0] = 0;
    gpr.dw[1] = 0;
}

void SPU::printState() {
    printf("pc: 0x%08x\n", state.pc);
    for (int i = 0; i < 128; i++) {
        printf("r%d: { 0x%08x, 0x%08x, 0x%08x, 0x%08x } 0x", i, state.gprs[i].w[3], state.gprs[i].w[2], state.gprs[i].w[1], state.gprs[i].w[0]);
        for (int j = 0; j < 16; j++)
            printf("%02x", state.gprs[i].b[15 - j]);
        printf("\n");
    }
}

template<typename T>
T SPU::read(u64 addr) {
    return Helpers::bswap<T>(*(T*)(&ls[addr]));
}
template u8  SPU::read(u64 addr);
template u16 SPU::read(u64 addr);
template u32 SPU::read(u64 addr);
template u64 SPU::read(u64 addr);

template<typename T>
void SPU::write(u64 addr, T data) {
    data = Helpers::bswap<T>(data);
    std::memcpy(&ls[addr], &data, sizeof(T));
}
template void SPU::write(u64 addr, u8  data);
template void SPU::write(u64 addr, u16 data);
template void SPU::write(u64 addr, u32 data);
template void SPU::write(u64 addr, u64 data);

SPUTypes::GPR SPU::read128(u64 addr) {
    SPUTypes::GPR data;
    data.dw[1] = read<u64>(addr);
    data.dw[0] = read<u64>(addr + 8);
    return data;
}

void SPU::write128(u64 addr, SPUTypes::GPR data) {
    write<u64>(addr, data.dw[1]);
    write<u64>(addr + 8, data.dw[0]);
}
