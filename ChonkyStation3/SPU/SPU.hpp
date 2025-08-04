#pragma once

#include <common.hpp>
#include <BitField.hpp>

#include <Memory.hpp>
#include <SPUTypes.hpp>


// Circular dependency
class PlayStation3;

class SPU {
public:
    SPU(PlayStation3* ps3) : ps3(ps3) {}
    PlayStation3* ps3;
    virtual int step(); // Returns number of cycles executed

    bool enabled = false;
    SPUTypes::State state;
    u8* ls;

    void clr(SPUTypes::GPR& gpr);   // Clears a register
    void printState();

    template<typename T> T read(u64 addr);
    template<typename T> void write(u64 addr, T data);
    SPUTypes::GPR read128(u64 addr);    // TODO: Rename GPR to a generic u128 type
    void write128(u64 addr, SPUTypes::GPR data);
};
