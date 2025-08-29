#include "FragmentShaderDecompiler.hpp"
#include <PlayStation3.hpp>


std::string FragmentShaderDecompiler::decompile(FragmentShader& shader_program) {
    std::string shader_base =
R"(#version 410 core


vec4 no_dest;
vec4 cc0;
vec4 cc1;

layout(location = 0) out vec4 out_col0;

uniform sampler2D tex;
uniform bool flip_tex;

)";
    std::string shader;
    std::string main = "";
    main.reserve(256_KB);
    for (int i = 0; i < 16; i++) used_inputs[i] = false;
    for (int i = 0; i < 256; i++) used_regs[i] = false;
    inputs = "";
    regs = "";
    constants = "";
    initialization = "";
    next_constant = 0;

    curr_offs = shader_program.addr;
    const bool float16_exports = !(shader_program.ctrl & 0x40); // CELL_GCM_SHADER_CONTROL_32_BITS_EXPORTS

    log("Decompiling fragment shader\n");

    while (true) {
        FragmentInstruction instr = fetchInstr(curr_offs);
        curr_offs += sizeof(FragmentInstruction);
        const u32 opc = instr.dst.opc | (instr.src1.branch << 6);
        if (!fragment_opcodes.contains(opc)) {
            log("ERROR: SKIPPED BAD OPC 0x%08x\n", opc);
            main += "// BAD OPC\n";
            continue;
        }
        log("%s (0x%08x)\n", fragment_opcodes[opc].c_str(), opc);
        
        if (opc == RSXFragment::NOP) continue;
        if (opc == RSXFragment::FENCT) continue;
        if (opc == RSXFragment::FENCB) continue;
        if (opc == RSXFragment::IFE) {
            main += "// IFE";
            continue;
        }
        if (opc == RSXFragment::LOOP) {
            main += "// LOOP";
            continue;
        }
        
        int num_lanes;
        const auto mask_str = mask(instr, num_lanes);
        const auto type = getType(num_lanes);
        std::string decompiled_dest = dest(instr);
        std::string decompiled_src;

        switch (opc) {
        case RSXFragment::MOV: {
            decompiled_src = std::format("{}", source(instr, 0));
            break;
        }
        case RSXFragment::MUL: {
            decompiled_src = std::format("({} * {})", source(instr, 0), source(instr, 1));
            break;
        }
        case RSXFragment::ADD: {
            decompiled_src = std::format("({} + {})", source(instr, 0), source(instr, 1));
            break;
        }
        case RSXFragment::MAD: {
            decompiled_src = std::format("(({} * {}) + {})", source(instr, 0), source(instr, 1), source(instr, 2));
            break;
        }
        case RSXFragment::DP3: {
            decompiled_src = std::format("vec4(dot(vec3({}), vec3({})))", source(instr, 0), source(instr, 1));
            break;
        }
        case RSXFragment::DP4: {
            decompiled_src = std::format("vec4(dot({}, {}))", source(instr, 0), source(instr, 1));
            break;
        }
        case RSXFragment::MIN: {
            decompiled_src = std::format("min({}, {})", source(instr, 0), source(instr, 1));
            break;
        }
        case RSXFragment::MAX: {
            decompiled_src = std::format("max({}, {})", source(instr, 0), source(instr, 1));
            break;
        }
        case RSXFragment::SLT: {
            decompiled_src = std::format("vec4(lessThan({}, {}))", source(instr, 0), source(instr, 1));
            break;
        }
        case RSXFragment::SGE: {
            decompiled_src = std::format("vec4(greaterThanEqual({}, {}))", source(instr, 0), source(instr, 1));
            break;
        }
        case RSXFragment::SLE: {
            decompiled_src = std::format("vec4(lessThanEqual({}, {}))", source(instr, 0), source(instr, 1));
            break;
        }
        case RSXFragment::SGT: {
            decompiled_src = std::format("vec4(greaterThan({}, {}))", source(instr, 0), source(instr, 1));
            break;
        }
        case RSXFragment::SEQ: {
            decompiled_src = std::format("vec4(equal({}, {}))", source(instr, 0), source(instr, 1));
            break;
        }
        case RSXFragment::FRC: {
            decompiled_src = std::format("fract({})", source(instr, 0));
            break;
        }
        case RSXFragment::FLR: {
            decompiled_src = std::format("floor({})", source(instr, 0));
            break;
        }
        case RSXFragment::KIL: {
            decompiled_src = "discard";
            break;
        }
        case RSXFragment::DDX: {
            decompiled_src = std::format("dFdx({})", source(instr, 0));
            break;
        }
        case RSXFragment::DDY: {
            decompiled_src = std::format("dFdy({})", source(instr, 0));
            break;
        }
        case RSXFragment::TEX: {
            if (instr.dst.tex_num == 0)
                decompiled_src = std::format("texture(tex, vec2({}.x, flip_tex ? (1.0f - {}.y) : {}.y))", source(instr, 0), source(instr, 0), source(instr, 0));
            else {
                log("WARNING: TEX NUM != 0\n");
                decompiled_src = "/* TODO: tex_num != 0 */ vec4(1.0f)";
            }
            break;
        }
                
        case RSXFragment::RCP: {
            decompiled_src = std::format("(1.0f / {}).xxxx", source(instr, 0));
            break;
        }
                
        case RSXFragment::RSQ: {
            decompiled_src = std::format("inversesqrt({}).xxxx", source(instr, 0));
            break;
        }
        case RSXFragment::EX2: {
            decompiled_src = std::format("vec4(exp2({}.x))", source(instr, 0));
            break;
        }
        case RSXFragment::LG2: {
            decompiled_src = std::format("vec4(log2({}.x))", source(instr, 0));
            break;
        }
        case RSXFragment::DP2: {
            decompiled_src = std::format("vec4(dot(vec2({}), vec2({})))", source(instr, 0), source(instr, 1));
            break;
        }
        case RSXFragment::TXB: {
            if (instr.dst.tex_num == 0)
                decompiled_src = std::format("/* TODO: TXB */ texture(tex, vec2({}))", source(instr, 0));
            else {
                log("WARNING: TEX NUM != 0\n");
                decompiled_src = "/* TODO: tex_num != 0 */ vec4(1.0f)";
            }
            break;
        }
        case RSXFragment::NRM: {
            decompiled_src = std::format("normalize(vec3({}))", source(instr, 0));
            break;
        }
        case RSXFragment::DIV: {
            decompiled_src = std::format("({} / {})", source(instr, 0), source(instr, 1));
            break;
        }
        case RSXFragment::DIVSQ: {
            decompiled_src = std::format("({} / sqrt({}))", source(instr, 0), source(instr, 1));
            break;
        }
                
        default:
            log("Shader so far:\n");
            log("%s\n", main.c_str());
            Helpers::panic("Unimplemented fragment instruction %s\n", fragment_opcodes[opc].c_str());
        }

        // Conditional execution
        if (!hasCond(instr)) {
            if (opc == RSXFragment::KIL)
                main += decompiled_src + ";";
            // TODO: The whole opc == TEX part might break some stuff.
            // I do this because on the RSX if you use single channel textures, when you sample them it expects the color to be broadcasted to all the lanes (I think?).
            // Because we use GL_RED to emulate this format, that doesn't happen (the color is only in the red channel), so stuff breaks.
            else if (opc != RSXFragment::TEX)
                main += std::format("{}{} = {}{};\n", decompiled_dest, mask_str, decompiled_src, mask_str);
            else
                main += std::format("{}{} = {}({});\n", decompiled_dest, mask_str, getType(num_lanes), decompiled_src);
        }
        else {
            // Swizzle condition
            const std::string all = "xyzw";
            std::string swizzle = "    ";
            swizzle[0] = all[instr.src0.cond_swizzle_x];
            swizzle[1] = all[instr.src0.cond_swizzle_y];
            swizzle[2] = all[instr.src0.cond_swizzle_z];
            swizzle[3] = all[instr.src0.cond_swizzle_w];
            std::string cc = instr.src0.cc_idx == 0 ? "cc0" : "cc1";
            std::string cond = getCond(instr);

            if (num_lanes == 1) {
                if (opc != RSXFragment::KIL)
                    main += std::format("if ({}.{} {} 0)\n\t{}{} = {}{};\n", cc, swizzle[0], cond, decompiled_dest, mask_str, decompiled_src, mask_str);
                else
                    main += std::format("if ({}.{} {} 0)\n\t{};\n", cc, swizzle[0], cond, decompiled_src);
            }
            else {
                for (int i = 0; i < num_lanes; i++) {
                    if (opc != RSXFragment::KIL)
                        main += std::format("if ({}.{} {} 0)\n\t{}.{} = {}.{};\n", cc, swizzle[i], cond, decompiled_dest, all[i], decompiled_src, all[i]);
                    else
                        main += std::format("if ({}.{} {} 0)\n\t{};\n", cc, swizzle[i], cond, decompiled_src);
                }
            }
        }

        curr_const = "";
        if (instr.dst.end) break;
    }

    std::string out_reg = !float16_exports ? "r0" : "h0";
    main += "\nout_col0 = " + out_reg + ";";
    
    declareFunction("void main", initialization + "\n" + main, shader);

    shader_base += inputs + "\n";
    shader_base += regs + "\n";
    shader_base += constants + "\n";
    shader_base += uniforms;
    std::string full_shader = shader_base + "\n\n" + shader;

    log("Decompiled fragment shader:\n");
    log("%s\n", full_shader.c_str());

    uniforms = "";
    uniform_names.clear();

    return full_shader;
}

FragmentInstruction FragmentShaderDecompiler::fetchInstr(u32 addr) {
    FragmentInstruction instr;
    instr.dst.raw   = fetch32(addr +  0);
    instr.src0.raw  = fetch32(addr +  4);
    instr.src1.raw  = fetch32(addr +  8);
    instr.src2.raw  = fetch32(addr + 12);
    return instr;
}

u32 FragmentShaderDecompiler::fetch32(u32 addr) {
    const auto data = ps3->mem.read<u32>(addr);
    return swap(data);
}

std::string FragmentShaderDecompiler::addConstant(float x, float y, float z, float w) {
    std::string name = "const" + std::to_string(next_constant++);
    constants += "const vec4 " + name + std::format(" = vec4({:f}f, {:f}f, {:f}f, {:f}f);\n", x, y, z, w);
    return name;
}

std::string FragmentShaderDecompiler::addUniform(u32 addr) {
    std::string name = std::format("uniform_{:x}", addr);
    
    // Did we already add this uniform?
    if (std::find(uniform_names.begin(), uniform_names.end(), name) != uniform_names.end())
        return name;
    
    uniform_names.push_back(name);
    uniforms += "uniform vec4 " + name + ";\n";
    log("Added uniform: %s\n", name.c_str());
    return name;
}

bool FragmentShaderDecompiler::isUniform(std::string name) {
    return std::find(uniform_names.begin(), uniform_names.end(), name) != uniform_names.end();
}

void FragmentShaderDecompiler::declareFunction(std::string name, std::string code, std::string& shader) {
    shader += name + "() {\n";
    std::istringstream stream(code);
    for (std::string line; std::getline(stream, line); )
        shader += "\t" + line + "\n";
    shader += "}\n";
}

void FragmentShaderDecompiler::markInputAsUsed(std::string name, int location) {
    if (used_inputs[location]) return;
    used_inputs[location] = true;
    inputs += "layout (location = " + std::to_string(location) + ") in vec4 " + name + ";\n";
}

void FragmentShaderDecompiler::markRegAsUsed(std::string name, int location) {
    if (used_regs[location]) return;
    used_regs[location] = true;
    std::string layout = "";
    regs += layout + "vec4 " + name + ";\n";
    initialization += name + " = vec4(0.0f, 0.0f, 0.0f, 1.0f);\n";
}

void FragmentShaderDecompiler::enableInput(u32 idx) {
    is_input[idx] = true;
    //log("Enabled input %d\n", idx);
}

std::string FragmentShaderDecompiler::source(FragmentInstruction& instr, int s) {
    std::string source = "";
    u32 type = 3;
    u8 x;
    u8 y;
    u8 z;
    u8 w;
    u32 src_idx;
    bool neg;
    bool abs;
    bool half;
    // TODO: is there a better way to do this...?
    switch (s) {
    case 0:
        type = instr.src0.type;
        x = instr.src0.x;
        y = instr.src0.y;
        z = instr.src0.z;
        w = instr.src0.w;
        src_idx = instr.src0.src_idx;
        neg = instr.src0.neg == 1;
        abs = instr.src0.abs == 1;
        half = instr.src0.half == 1;
        break;
    case 1:
        type = instr.src1.type;
        x = instr.src1.x;
        y = instr.src1.y;
        z = instr.src1.z;
        w = instr.src1.w;
        src_idx = instr.src1.src_idx;
        neg = instr.src1.neg == 1;
        abs = instr.src1.abs == 1;
        half = instr.src1.half == 1;
        break;
    case 2:
        type = instr.src2.type;
        x = instr.src2.x;
        y = instr.src2.y;
        z = instr.src2.z;
        w = instr.src2.w;
        src_idx = instr.src2.src_idx;
        neg = instr.src2.neg == 1;
        abs = instr.src2.abs == 1;
        half = instr.src2.half == 1;
        break;
    }
    
    switch (type) {
    case FRAGMENT_SOURCE_TYPE::TEMP: {
        source = (!half ? "r" : "h") + std::to_string(src_idx);
        markRegAsUsed(source, src_idx + (half ? 48 : 0));
        break;
    }
    case FRAGMENT_SOURCE_TYPE::INPUT: {
        const u32 idx = instr.src2.use_index_reg ? (instr.src2.addr_reg + 4) : instr.dst.src_idx.Value();
        source = input_names[idx];
        markInputAsUsed(source, idx);
        break;
    }

    case FRAGMENT_SOURCE_TYPE::CONST: {
        // Was an uniform uploaded to this address?
        const std::string uniform_name = std::format("uniform_{:x}", curr_offs);
        if (isUniform(uniform_name)) {
            source = uniform_name;
            break;
        }

        // Normal constant
        
        // Check if this instruction already has a constant
        if (!curr_const.empty()) {
            source = curr_const;
            break;
        }
        
        // Fetch new constant
        u32 w0 = fetch32(curr_offs +  0);
        u32 w1 = fetch32(curr_offs +  4);
        u32 w2 = fetch32(curr_offs +  8);
        u32 w3 = fetch32(curr_offs + 12);
        curr_offs += 4 * sizeof(u32);
        float x = reinterpret_cast<float&>(w0);
        float y = reinterpret_cast<float&>(w1);
        float z = reinterpret_cast<float&>(w2);
        float w = reinterpret_cast<float&>(w3);
        source = addConstant(x, y, z, w);
        curr_const = source;
        break;
    }

    default:
        Helpers::panic("Unimplemented source type %d\n", type);
    }

    // Swizzle
    // Each field in the src contains a value ranging from 0 to 3, each corrisponding to a lane
    const std::string all = "xyzw";
    std::string swizzle = "    ";
    swizzle[0] = all[x];
    swizzle[1] = all[y];
    swizzle[2] = all[z];
    swizzle[3] = all[w];

    // We can omit ".xyzw"
    if (swizzle != all)
        source += "." + swizzle;

    if (abs)
        source = "abs(" + source + ")";
    if (neg)
        source = "(-" + source + ")";

    return source;
}

std::string FragmentShaderDecompiler::dest(FragmentInstruction& instr) {
    // TODO: there is a lot more than this
    std::string dest; 
    if (!instr.dst.no_dest) {
        dest = (!instr.dst.half ? "r" : "h") + std::to_string(instr.dst.dest_idx);
        markRegAsUsed(dest, instr.dst.dest_idx + (instr.dst.half ? 48 : 0));
    }
    else {
        if (instr.dst.set_cc) {
            if (instr.src0.set_cc_idx == 1) Helpers::panic("dest: unimplemented set cc1\n");
            dest = "cc0";
        }
        else {
            dest = "no_dest";
        }
    }
    return dest;
}

std::string FragmentShaderDecompiler::mask(FragmentInstruction& instr, int& num_lanes) {
    std::string mask = ".";
    const std::string full = ".xyzw";
    num_lanes = 0;

    if (instr.dst.x) {
        mask += "x";
        num_lanes++;
    }
    if (instr.dst.y) {
        mask += "y";
        num_lanes++;
    }
    if (instr.dst.z) {
        mask += "z";
        num_lanes++;
    }
    if (instr.dst.w) {
        mask += "w";
        num_lanes++;
    }

    if (mask == full) mask = "";
    else if (mask == ".") {
        mask = "";
        num_lanes = 4;
    }
    return mask;
}

std::string FragmentShaderDecompiler::getType(const int num_lanes) {
    switch (num_lanes) {
    case 1: return "float";
    case 2: return "vec2";
    case 3: return "vec3";
    case 4: return "vec4";
    default:
        Helpers::panic("getType: %d lanes\n", num_lanes);
    }
}

std::string FragmentShaderDecompiler::getCond(FragmentInstruction& instr) {
    if      (instr.src0.cond_eq && instr.src0.cond_lt) return "<=";
    else if (instr.src0.cond_eq && instr.src0.cond_gt) return ">=";
    else if (instr.src0.cond_lt && instr.src0.cond_gt) return "!=";
    else if (instr.src0.cond_lt) return "<";
    else if (instr.src0.cond_gt) return ">";
    else if (instr.src0.cond_eq) return "==";
    else Helpers::panic("condition can never be met\n");  // Are there going to be games that do this?
}

bool FragmentShaderDecompiler::hasCond(FragmentInstruction& instr) {
    return !(instr.src0.cond_lt && instr.src0.cond_eq && instr.src0.cond_gt);
}
