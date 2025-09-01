#pragma once

#include <BitField.hpp>


struct FragmentInstruction {
    union {
        u32 raw;
        BitField<0,  1, u32> end;
        BitField<1,  6, u32> dest_idx;
        BitField<7,  1, u32> half;
        BitField<8,  1, u32> set_cc;
        BitField<9,  1, u32> x;
        BitField<10, 1, u32> y;
        BitField<11, 1, u32> z;
        BitField<12, 1, u32> w;
        BitField<13, 4, u32> src_idx;
        BitField<17, 4, u32> tex_num;
        BitField<24, 6, u32> opc;
        BitField<30, 1, u32> no_dest;
        BitField<31, 1, u32> saturate;
    } dst;
    union {
        u32 raw;
        BitField<0,  2, u32> type;
        BitField<2,  6, u32> src_idx;
        BitField<8,  1, u32> half;
        BitField<9,  2, u32> x;
        BitField<11, 2, u32> y;
        BitField<13, 2, u32> z;
        BitField<15, 2, u32> w;
        BitField<17, 1, u32> neg;
        BitField<18, 1, u32> cond_lt;
        BitField<19, 1, u32> cond_eq;
        BitField<20, 1, u32> cond_gt;
        BitField<21, 2, u32> cond_swizzle_x;
        BitField<23, 2, u32> cond_swizzle_y;
        BitField<25, 2, u32> cond_swizzle_z;
        BitField<27, 2, u32> cond_swizzle_w;
        BitField<29, 1, u32> abs;
        BitField<30, 1, u32> set_cc_idx;
        BitField<31, 1, u32> cc_idx;
    } src0;
    union {
        u32 raw;
        BitField<0,  2, u32> type;
        BitField<2,  6, u32> src_idx;
        BitField<8,  1, u32> half;
        BitField<9,  2, u32> x;
        BitField<11, 2, u32> y;
        BitField<13, 2, u32> z;
        BitField<15, 2, u32> w;
        BitField<17, 1, u32> neg;
        BitField<18, 1, u32> abs;
        BitField<31, 1, u32> branch;
    } src1;
    union {
        u32 raw;
        BitField<0,  2, u32> type;
        BitField<2,  6, u32> src_idx;
        BitField<8,  1, u32> half;
        BitField<9,  2, u32> x;
        BitField<11, 2, u32> y;
        BitField<13, 2, u32> z;
        BitField<15, 2, u32> w;
        BitField<17, 1, u32> neg;
        BitField<18, 1, u32> abs;
        BitField<19, 11, u32> addr_reg;
        BitField<30, 1, u32> use_index_reg;
        BitField<31, 1, u32> perspective_correction;
    } src2;
};
