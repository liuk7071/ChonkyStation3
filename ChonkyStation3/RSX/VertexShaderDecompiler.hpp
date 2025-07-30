#pragma once

#include <common.hpp>
#include <logger.hpp>
#include <BitField.hpp>

#include <string>
#include <sstream>
#include <unordered_map>
#include <format>
#include <algorithm>

#include <RSXShaderOpcodes.hpp>


class VertexShaderDecompiler {
public:
    struct VertexInstruction {
        union {
            u32 raw;
            BitField<0,  2, u32> addr_swizzle;
            BitField<2,  2, u32> cond_swizzle_w;
            BitField<4,  2, u32> cond_swizzle_z;
            BitField<6,  2, u32> cond_swizzle_y;
            BitField<8,  2, u32> cond_swizzle_x;
            BitField<10, 3, u32> cond;
            BitField<13, 1, u32> cond_enable;
            BitField<15, 6, u32> temp_dst_idx;
            BitField<21, 1, u32> src0_abs;
            BitField<22, 1, u32> src1_abs;
            BitField<23, 1, u32> src2_abs;
            BitField<24, 1, u32> addr_reg_sel;
            BitField<25, 1, u32> cond_reg_sel;
            BitField<26, 1, u32> saturate;
            BitField<29, 1, u32> cond_enable_1;
            BitField<30, 1, u32> is_output;
        } w0;
        union {
            u32 raw;
            BitField<0,  8,  u32> src0_hi;
            BitField<8,  4,  u32> input_src_idx;
            BitField<12, 10, u32> const_src_idx;
            BitField<22, 5,  u32> vector_opc;
            BitField<27, 5,  u32> scalar_opc;
        } w1;
        union {
            u32 raw;
            BitField<0, 6,  u32> src2_hi;
            BitField<6, 17, u32> src1;
            BitField<23, 9, u32> src0_lo;
        } w2;
        union {
            u32 raw;
            BitField<0,  1,  u32> end;
            BitField<1,  1,  u32> is_indexed_const;
            BitField<2,  5,  u32> dst;
            BitField<7,  6,  u32> sca_temp_dst_idx;
            BitField<13, 1,  u32> w;
            BitField<14, 1,  u32> z;
            BitField<15, 1,  u32> y;
            BitField<16, 1,  u32> x;
            BitField<17, 1,  u32> sca_w;
            BitField<18, 1,  u32> sca_z;
            BitField<19, 1,  u32> sca_y;
            BitField<20, 1,  u32> sca_x;
            BitField<21, 11, u32> src2_lo;
        } w3;
    };

    union VertexSource {
        u32 raw;
        BitField<0,  2, u32> type;
        BitField<2,  6, u32> temp_src_idx;
        BitField<8,  2, u32> w;
        BitField<10, 2, u32> z;
        BitField<12, 2, u32> y;
        BitField<14, 2, u32> x;
        BitField<16, 1, u32> neg;
    };
    
    struct SourceIndexPair {
        int idx;
        VertexSource src;
    };

    MAKE_LOG_FUNCTION(log, vertex_shader);

    std::string decompile(u32* shader_data, u32 start);
    
    void declareFunction(std::string name, std::string code, std::string& shader);
    void markInputAsUsed(std::string name, int location);
    void markOutputAsUsed(std::string name, int location);

    std::string source(SourceIndexPair& src_idx, VertexInstruction* instr);
    std::string dest(VertexInstruction* instr, bool is_addr_reg = false, bool is_scalar = false);
    std::string mask(VertexInstruction* instr, int& num_lanes, bool is_scalar = false);
    std::string getType(const int num_lanes);

    bool used_inputs[16];
    bool used_outputs[16];
    std::string inputs;
    std::string outputs;
    std::string constants;
    std::string initialization;

    enum VERTEX_SOURCE_TYPE {
        TEMP = 1,
        INPUT = 2,
        CONST = 3
    };

    const std::string input_names[16] = {
        "vs_pos",
        "vs_weight",
        "vs_normal",
        "vs_diff_color",
        "vs_spec_color",
        "vs_fog",
        "vs_point_size",
        "vs_unk7",
        "vs_tex0",
        "vs_tex1",
        "vs_tex2",
        "vs_tex3",
        "vs_tex4",
        "vs_tex5",
        "vs_tex6",
        "vs_tex7"
    };

    const std::string output_names[16] = {
        "fs_pos",
        "fs_col0",
        "fs_col1",
        "fs_bfc0",
        "fs_bfc1",
        "fs_fog",
        "fs_point_size",
        "fs_tex0",
        "fs_tex1",
        "fs_tex2",
        "fs_tex3",
        "fs_tex4",
        "fs_tex5",
        "fs_tex6",
        "fs_tex7",
        "fs_unk15"
    };

    // Above 20 means I still have to figure out what the fuck it is
    const u32 output_locations_map[16] = {
        0,
        1,
        2,
        20,
        21,
        3,
        22,
        4,
        5,
        6,
        7,
        8,
        9,
        10,
        11,
        23
    };

    std::unordered_map<u8, std::string> vertex_vector_opcodes {
        { RSXVertex::VECTOR::NOP, "NOP" },
        { RSXVertex::VECTOR::MOV, "MOV" },
        { RSXVertex::VECTOR::MUL, "MUL" },
        { RSXVertex::VECTOR::ADD, "ADD" },
        { RSXVertex::VECTOR::MAD, "MAD" },
        { RSXVertex::VECTOR::DP3, "DP3" },
        { RSXVertex::VECTOR::DPH, "DPH" },
        { RSXVertex::VECTOR::DP4, "DP4" },
        { RSXVertex::VECTOR::DST, "DST" },
        { RSXVertex::VECTOR::MIN, "MIN" },
        { RSXVertex::VECTOR::MAX, "MAX" },
        { RSXVertex::VECTOR::SLT, "SLT" },
        { RSXVertex::VECTOR::SGE, "SGE" },
        { RSXVertex::VECTOR::ARL, "ARL" },
        { RSXVertex::VECTOR::FRC, "FRC" },
        { RSXVertex::VECTOR::FLR, "FLR" },
        { RSXVertex::VECTOR::SEQ, "SEQ" },
        { RSXVertex::VECTOR::SFL, "SFL" },
        { RSXVertex::VECTOR::SGT, "SGT" },
        { RSXVertex::VECTOR::SLE, "SLE" },
        { RSXVertex::VECTOR::SNE, "SNE" },
        { RSXVertex::VECTOR::STR, "STR" },
        { RSXVertex::VECTOR::SSG, "SSG" },
        { RSXVertex::VECTOR::TXL, "TXL" },
    };

    std::unordered_map<u8, std::string> vertex_scalar_opcodes {
        { RSXVertex::SCALAR::NOP, "NOP" },
        { RSXVertex::SCALAR::MOV, "MOV" },
        { RSXVertex::SCALAR::RCP, "RCP" },
        { RSXVertex::SCALAR::RCC, "RCC" },
        { RSXVertex::SCALAR::RSQ, "RSQ" },
        { RSXVertex::SCALAR::EXP, "EXP" },
        { RSXVertex::SCALAR::LOG, "LOG" },
        { RSXVertex::SCALAR::LIT, "LIT" },
        { RSXVertex::SCALAR::BRA, "BRA" },
        { RSXVertex::SCALAR::BRI, "BRI" },
        { RSXVertex::SCALAR::CAL, "CAL" },
        { RSXVertex::SCALAR::CLI, "CLI" },
        { RSXVertex::SCALAR::RET, "RET" },
        { RSXVertex::SCALAR::LG2, "LG2" },
        { RSXVertex::SCALAR::EX2, "EX2" },
        { RSXVertex::SCALAR::SIN, "SIN" },
        { RSXVertex::SCALAR::COS, "COS" },
        { RSXVertex::SCALAR::BRB, "BRB" },
        { RSXVertex::SCALAR::CLB, "CLB" },
        { RSXVertex::SCALAR::PSH, "PSH" },
        { RSXVertex::SCALAR::POP, "POP" },
    };
};
