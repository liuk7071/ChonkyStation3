#include "RSX.hpp"
#include "PlayStation3.hpp"


RSX::RSX(PlayStation3* ps3) : ps3(ps3), gcm(ps3->module_manager.cellGcmSys) {
    OpenGL::setViewport(1280, 720);     // TODO: Get resolution from cellVideoOut
    OpenGL::setClearColor(0.0f, 0.0f, 0.0f, 1.0f);
    OpenGL::clearColor();
}

u32 RSX::fetch32() {
    u32 data = ps3->mem.read<u32>(gcm.gcm_config.io_addr + curr_cmd);
    curr_cmd += 4;
    gcm.ctrl->get = gcm.ctrl->get + 4;  // Didn't overload += in BEField
    return data;
}

void RSX::runCommandList() {
    int cmd_count = gcm.ctrl->put - gcm.ctrl->get;
    if (cmd_count <= 0) return;

    printf("Executing commands (%d bytes)\n", cmd_count);

    // Execute while get < put
    // We increment get as we fetch data from the FIFO
    curr_cmd = 0;
    while (gcm.ctrl->get < gcm.ctrl->put) {
        u32 cmd = fetch32();
        const auto cmd_num = cmd & 0x3ffff;
        const auto argc = (cmd >> 18) & 0x7ff;

        if (cmd & 0x20000000) { // jump
            curr_cmd = cmd & ~0x20000000;
            continue;
        }

        std::vector<u32> args;
        for (int i = 0; i < argc; i++)
            args.push_back(fetch32());

        if (command_names.contains(cmd_num))
            printf("%s\n", command_names[cmd_num].c_str());

        switch (cmd & 0x3ffff) {

        case NV406E_SEMAPHORE_OFFSET:
        case NV4097_SET_SEMAPHORE_OFFSET: {
            semaphore_offset = args[0];
            break;
        }

        case NV4097_BACK_END_WRITE_SEMAPHORE_RELEASE: {
            const u32 val = (args[0] & 0xff00ff00) | ((args[0] & 0xff) << 16) | ((args[0] >> 16) & 0xff);
            ps3->mem.write<u32>(gcm.label_addr + semaphore_offset, val);
            break;
        }

        case NV406E_SEMAPHORE_ACQUIRE: {
            break;
        }

        case NV4097_SET_TRANSFORM_PROGRAM: {
            for (auto& i : args) vertex_shader_data.push_back(i);
            printf("Uploading %d words (%d instructions)\n", args.size(), args.size() / 4);
            break;
        }

        case NV4097_DRAW_INDEX_ARRAY: {
            auto vertex_shader = shader_decompiler.decompileVertex(vertex_shader_data);
            auto fragment_shader = shader_decompiler.decompileFragment(fragment_shader_data);
            printf("Compiled vertex shader:\n");
            puts(vertex_shader.c_str());
            printf("Compiled fragment shader:\n");
            puts(fragment_shader.c_str());

            OpenGL::Shader vertex, fragment;
            OpenGL::Program program;
            vertex.create(vertex_shader, OpenGL::ShaderType::Vertex);
            fragment.create(fragment_shader, OpenGL::ShaderType::Fragment);
            program.create({ vertex, fragment });
            program.use();
            break;
        }

        case NV4097_SET_COLOR_CLEAR_VALUE: {
            clear_color.r() = ((args[0] >>  0) & 0xff) / 255.0f;
            clear_color.g() = ((args[0] >>  8) & 0xff) / 255.0f;
            clear_color.b() = ((args[0] >> 16) & 0xff) / 255.0f;
            clear_color.a() = ((args[0] >> 24) & 0xff) / 255.0f;
            break;
        }

        case NV4097_CLEAR_SURFACE: {
            OpenGL::setClearColor(clear_color.r(), clear_color.g(), clear_color.b(), clear_color.a());
            OpenGL::clearColor();
            OpenGL::clearDepthAndStencil();
            break;
        }

        case NV4097_SET_TRANSFORM_PROGRAM_LOAD: {
            Helpers::debugAssert(args[0] == 0, "Set transform program load address: addr != 0\n");
            vertex_shader_data.clear();
            break;
        }

        default:
            if (command_names.contains(cmd & 0x3ffff))
                printf("Unimplemented RSX command %s\n", command_names[cmd_num].c_str());
            else
                printf("Unimplemented RSX command 0x%08x (0x%08x)\n", cmd_num, cmd);
        }
    }
}