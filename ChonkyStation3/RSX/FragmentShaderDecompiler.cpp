#include "FragmentShaderDecompiler.hpp"
#include <PlayStation3.hpp>


std::string FragmentShaderDecompiler::decompile(FragmentShader& shader_program) {
    std::string shader_base =
R"(
#version 410 core


vec4 unimpl;

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

    log("Decompiling fragment shader\n");

    while (true) {
        FragmentInstruction instr = fetchInstr(curr_offs);
        curr_offs += sizeof(FragmentInstruction);
        const u32 opc = instr.dst.opc | (instr.src1.branch << 6);
        log("%s\n", fragment_opcodes[opc].c_str());

        if (opc == RSXFragment::NOP) continue;

        int num_lanes;
        const auto mask_str = mask(instr, num_lanes);
        const auto type = getType(num_lanes);

        switch (opc) {
        case RSXFragment::MOV: {
            main += std::format("{}{} = {}({});\n", dest(instr), mask_str, type, source(instr, 0));
            break;
        }
        case RSXFragment::MUL: {
            main += std::format("{}{} = {}({} * {});\n", dest(instr), mask_str, type, source(instr, 0), source(instr, 1));
            break;
        }
        case RSXFragment::ADD: {
            main += std::format("{}{} = {}({} + {});\n", dest(instr), mask_str, type, source(instr, 0), source(instr, 1));
            break;
        }
        case RSXFragment::MAD: {
            main += std::format("{}{} = {}(({} * {}) + {});\n", dest(instr), mask_str, type, source(instr, 0), source(instr, 1), source(instr, 2));
            break;
        }
        case RSXFragment::DP3: {
            main += std::format("{}{} = {}(dot(vec3({}), vec3({})));\n", dest(instr), mask_str, type, source(instr, 0), source(instr, 1));
            break;
        }
        case RSXFragment::MAX: {
            main += std::format("{}{} = {}(max({}, {}));\n", dest(instr), mask_str, type, source(instr, 0), source(instr, 1));
            break;
        }
        case RSXFragment::EX2: {
            main += std::format("{}{} = {}(exp2({}));\n", dest(instr), mask_str, type, source(instr, 0));
            break;
        }
        case RSXFragment::LG2: {
            main += std::format("{}{} = {}(log2({}));\n", dest(instr), mask_str, type, source(instr, 0));
            break;
        }
        case RSXFragment::NRM: {
            main += std::format("{}{} = {}(normalize(vec3({})));\n", dest(instr), mask_str, type, source(instr, 0));
            break;
        }

        default:
            log("Shader so far:\n");
            log("%s\n", main.c_str());
            Helpers::panic("Unimplemented fragment instruction %s\n", fragment_opcodes[opc].c_str());
        }

        if (instr.dst.end) break;
    }

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

FragmentShaderDecompiler::FragmentInstruction FragmentShaderDecompiler::fetchInstr(u32 addr) {
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
    uniforms += "uniform vec4 " + name + ";\n";
    uniform_names.push_back(name);
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
    if (location == 0)
        layout = "layout (location = 0) out ";
    regs += layout + "vec4 " + name + ";\n";
    initialization += name + " = vec4(0.0f, 0.0f, 0.0f, 1.0f);\n";
}

void FragmentShaderDecompiler::enableInput(u32 idx) {
    is_input[idx] = true;
    log("Enabled input %d\n", idx);
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
        break;
    }
    
    switch (type) {
    case FRAGMENT_SOURCE_TYPE::TEMP: {
        source = "r" + std::to_string(src_idx);
        markRegAsUsed(source, src_idx);
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
        dest = "r" + std::to_string(instr.dst.dest_idx);
        markRegAsUsed(dest, instr.dst.dest_idx);
    }
    else {
        dest = "unimpl";
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