#include "RSX.hpp"
#include "PlayStation3.hpp"


RSX::RSX(PlayStation3* ps3) : ps3(ps3), gcm(ps3->module_manager.cellGcmSys), fragment_shader_decompiler(ps3) {
    std::memset(constants, 0, 512 * 4);
    last_tex.addr = 0;
    last_tex.format = 0;
    last_tex.width = 0;
    last_tex.height = 0;
    vertex_array.bindings.resize(16);
}

void RSX::initGL() {
    OpenGL::setViewport(1280, 720);     // TODO: Get resolution from cellVideoOut
    OpenGL::setClearColor(0.0f, 0.0f, 0.0f, 1.0f);
    OpenGL::clearColor();
    OpenGL::clearDepth();
    vao.create();
    vbo.create();
    vao.bind();
    vbo.bind();
    glGenBuffers(1, &ibo);
    glGenBuffers(1, &quad_ibo);

    OpenGL::setDepthFunc(OpenGL::DepthFunc::Lequal);
    OpenGL::disableScissor();
    OpenGL::setFillMode(OpenGL::FillMode::FillPoly);
    OpenGL::setBlendEquation(OpenGL::BlendEquation::Add);
    //glFrontFace(GL_CW);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, quad_ibo);
    quad_index_array.push_back(0);
    quad_index_array.push_back(1);
    quad_index_array.push_back(2);
    quad_index_array.push_back(2);
    quad_index_array.push_back(3);
    quad_index_array.push_back(0);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, quad_index_array.size() * 4, quad_index_array.data(), GL_STATIC_DRAW);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ibo);

    fb.create();

    // Create depth texture
    glGenTextures(1, &depth_tex.m_handle);
    glBindTexture(GL_TEXTURE_2D, depth_tex.m_handle);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT24, 1280, 720, 0, GL_DEPTH_COMPONENT, GL_FLOAT, (void*)0);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

    fb.bind(GL_FRAMEBUFFER);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, depth_tex.m_handle, 0);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    
    // Setup vertex constant UBO
    glGenBuffers(1, &vertex_consts_ubo);
    glBindBuffer(GL_UNIFORM_BUFFER, vertex_consts_ubo);
    glBufferData(GL_UNIFORM_BUFFER, 468 * 4 * sizeof(float), (void*)0, GL_STATIC_DRAW);
    glBindBufferBase(GL_UNIFORM_BUFFER, 10, vertex_consts_ubo);
}

void RSX::setEaTableAddr(u32 addr) {
    log("Set offset table addr: 0x%08x\n", addr);
    ea_table = addr;
}

u32 RSX::ioToEa(u32 offs) {
    return ((u32)ps3->mem.read<u16>(ea_table + ((offs >> 20) * 2)) << 20) | (offs & 0xfffff);
}

u32 RSX::fetch32() {
    const u32 addr = gcm.ctrl->get;
    u32 data = ps3->mem.read<u32>(ioToEa(addr));
    gcm.ctrl->get = gcm.ctrl->get + 4;  // Didn't overload += in BEField
    return data;
}

u32 RSX::offsetAndLocationToAddress(u32 offset, u8 location) {
    if (location == 0) return ps3->module_manager.cellGcmSys.gcm_config.local_addr + offset;
    else return ioToEa(offset);
}

void RSX::compileProgram() {
    RSXCache::CachedShader cached_shader;
    // Check if our shaders were cached
    const u64 hash_vertex = cache.computeHash((u8*)&vertex_shader_data[vertex_shader_start_idx * 4], 512 * 4 - vertex_shader_start_idx * 4);
    if (!cache.getShader(hash_vertex, cached_shader)) {
        // Shader wasn't cached, compile it and add it to the cache
        auto vertex_shader = vertex_shader_decompiler.decompile(vertex_shader_data, vertex_shader_start_idx);
        OpenGL::Shader new_shader;
        if(!new_shader.create(vertex_shader, OpenGL::ShaderType::Vertex))
            Helpers::panic("Failed to create vertex shader object");
        cache.cacheShader(hash_vertex, { new_shader });
        vertex = new_shader;
    }
    else {
        vertex = cached_shader.shader;
    }

    const u64 hash_fragment = cache.computeHash(fragment_shader_program.getData(ps3->mem), fragment_shader_program.getSize(ps3->mem));
    if (!cache.getShader(hash_fragment, cached_shader)) {
        // Shader wasn't cached, compile it and add it to the cache
        auto fragment_shader = fragment_shader_decompiler.decompile(fragment_shader_program);
        OpenGL::Shader new_shader;
        if(!new_shader.create(fragment_shader, OpenGL::ShaderType::Fragment))
            Helpers::panic("Failed to create fragment shader object");;
        cache.cacheShader(hash_fragment, { new_shader });
        fragment = new_shader;
    }
    else {
        fragment = cached_shader.shader;
    }

    // Check if our shader program was cached
    const u64 hash_program = cache.computeProgramHash(hash_vertex, hash_fragment);
    if (!cache.getProgram(hash_program, program)) {
        // Program wasn't cached, link it and add it to the cache
        OpenGL::Program new_program;
        new_program.create({ vertex, fragment });
        program = new_program;
        program.use();

        // Texture samplers
        const int loc = glGetUniformLocation(program.handle(), "tex");
        glUniform1i(loc, 0);
        
        // Vertex constants uniform buffer
        const int block_idx = glGetUniformBlockIndex(program.handle(), "VertexConstants");
        glUniformBlockBinding(program.handle(), block_idx, 10);

        // Cache it
        cache.cacheProgram(hash_program, new_program);
    }
    else {
        program.use();
    }
    
    program_changed = last_program_hash != hash_program;
    last_program_hash = hash_program;
}

void RSX::setupVAO() {
    log("Vertex configuration:\n");
    for (auto& binding : vertex_array.bindings) {
        if (!binding.size) continue;
        log("Attribute %d: size: %d, stride %d, type: %d, offs: 0x%08x\n", binding.index, binding.size, binding.stride, binding.type, binding.offset);
        u32 offs_in_buf = binding.offset - vertex_array.getBase();
        // Setup VAO attribute
        switch (binding.type) {
        case 1:
            vao.setAttributeFloat<GLshort>(binding.index, binding.size, binding.stride, (void*)offs_in_buf, true);
            break;
        case 6:
            log("TODO: CMP ATTRIBUTE TYPE\n");
            // fallthrough
        case 2:
            vao.setAttributeFloat<float>(binding.index, binding.size, binding.stride, (void*)offs_in_buf, false);
            break;
        case 4:
            vao.setAttributeFloat<GLubyte>(binding.index, binding.size, binding.stride, (void*)offs_in_buf, true);
            break;
        case 5:
            vao.setAttributeFloat<GLshort>(binding.index, binding.size, binding.stride, (void*)offs_in_buf, false);
            break;
        case 7:
            vao.setAttributeFloat<GLubyte>(binding.index, binding.size, binding.stride, (void*)offs_in_buf, false);
            break;
        default:
            Helpers::panic("Unimplemented vertex attribute type %d\n", binding.type);
        }
        vao.enableAttribute(binding.index);
    }
}

template <bool is_inline_array>
void RSX::getVertices(u32 n_vertices, std::vector<u8>& vtx_buf, u32 start) {
    u32 vtx_buf_offs = vtx_buf.size();
    vtx_buf.resize(vtx_buf_offs + (vertex_array.size() * (n_vertices + start)));

    auto fetch = [this]<typename T, bool inlined>(u32 offs, u32 base) -> T {
        if constexpr (!inlined)
            return ps3->mem.read<T>(base + offs);
        else return *(T*)&((u8*)inline_array.data())[offs];
    };

    for (auto& binding : vertex_array.bindings) {
        if (!binding.size) continue;
        const u32 base = vertex_array.getBase();
        const u32 offs = binding.offset;
        const u32 offs_in_buf = offs - base;
        const auto size = binding.sizeOfComponent();

        // offs_in_buf + i * binding.stride + j * size
        // - offs_in_buf    : offset of the first attribute of this binding relative to the base of the vertex array
        // - i              : vertex index
        // - binding.stride : distance in bytes between 2 attributes of this binding
        // - j              : attribute element ranging from 0 to binding.size
        // - size:          : size of 1 element of this attribute

        // Collect vertex data
        for (int i = start; i < start + n_vertices; i++) {
            for (int j = 0; j < binding.size; j++) {
                switch (binding.type) {
                case 6:
                    log("TODO: CMP ATTRIBUTE TYPE\n");
                    // fallthrough
                case 2: {
                    u32 x = fetch.template operator()<u32, is_inline_array>(offs_in_buf + i * binding.stride + j * size, base);
                    *(float*)&vtx_buf[vtx_buf_offs + offs_in_buf + binding.stride * i + j * size] = reinterpret_cast<float&>(x);
                    break;
                }
                case 7:
                case 4: {
                    u8 x = fetch.template operator()<u8, is_inline_array>(offs_in_buf + i * binding.stride + j * size, base);
                    vtx_buf[vtx_buf_offs + offs_in_buf + binding.stride * i + j * size] = x;
                    break;
                }
                case 1:
                case 5: {
                    u16 x = fetch.template operator()<u16, is_inline_array>(offs_in_buf + i * binding.stride + j * size, base);
                    *(u16*)&vtx_buf[vtx_buf_offs + offs_in_buf + binding.stride * i + j * size] = x;
                    break;
                }
                default:
                    Helpers::panic("Unimplemented vertex attribute type %d\n", binding.type);
                }
            }
            //log("x: %f y: %f z: %f\n", *(float*)&vtx_buf[binding.stride * i + 0], *(float*)&vtx_buf[binding.stride * i + 4], *(float*)&vtx_buf[binding.stride * i + 8]);
        }
    }
}

void RSX::uploadVertexConstants() {
    // Viewport data
    if (viewport_offs_dirty || program_changed) {
        viewport_offs_dirty = false;
        glUniform3f(glGetUniformLocation(program.handle(), "viewport_offs"), viewport_offs[0], viewport_offs[1], viewport_offs[2]);
    }
    if (viewport_scale_dirty || program_changed) {
        viewport_scale_dirty = false;
        glUniform3f(glGetUniformLocation(program.handle(), "viewport_scale"), viewport_scale[0], viewport_scale[1], viewport_scale[2]);
    }
    if (surface_clip_dirty || program_changed) {
        surface_clip_dirty = false;
        glUniform2i(glGetUniformLocation(program.handle(), "surface_clip"), surface_clip[0], surface_clip[1]);
    }

    if (!constants_dirty) return;
    constants_dirty = false;
    
    // Constants
    glBindBuffer(GL_UNIFORM_BUFFER, vertex_consts_ubo); // I don't think I need to rebind this here
    glBufferSubData(GL_UNIFORM_BUFFER, 0, 468 * 4 * sizeof(float), constants);
}

void RSX::uploadFragmentUniforms() {
    // Fragment uniforms
    for (auto& i : fragment_uniforms) {
        glUniform4f(glGetUniformLocation(program.handle(), i.name.c_str()), i.x, i.y, i.z, i.w);
    }
    fragment_uniforms.clear();

    glUniform1i(glGetUniformLocation(program.handle(), "flip_tex"), should_flip_tex ? GL_TRUE : GL_FALSE);
}

void RSX::uploadTexture() {
    // Don't do anything if the current texture is the same as the last one
    // TODO: This will break if a game uploads a different texture but with the same format, width and height to the same address as the previous texture.
    // I'm unsure how common that is. Probably make this toggleable in the future in case some games break
    if (texture == last_tex) {
       return;
    }

    auto swizzle = [this]() {
        // should_flip_tex == framebuffer texture
        // We don't reverse the swizzling because the framebuffer textures are written in the right order
        const bool rev = getRawTextureFormat(texture.format) == CELL_GCM_TEXTURE_A8R8G8B8 && !should_flip_tex;
        tex_swizzle_a = swizzle_map[rev ? 3 - (control1 & 3) : (control1 & 3)];
        tex_swizzle_r = swizzle_map[rev ? 3 - ((control1 >> 2) & 3) : ((control1 >> 2) & 3)];
        tex_swizzle_g = swizzle_map[rev ? 3 - ((control1 >> 4) & 3) : ((control1 >> 4) & 3)];
        tex_swizzle_b = swizzle_map[rev ? 3 - ((control1 >> 6) & 3) : ((control1 >> 6) & 3)];
        // TODO: Use a dirty flag and only update the swizzle when it's changed?
        u8 raw_fmt = getRawTextureFormat(texture.format);
        if (raw_fmt != CELL_GCM_TEXTURE_B8 && raw_fmt != CELL_GCM_TEXTURE_X16 && raw_fmt != CELL_GCM_TEXTURE_X32_FLOAT) {
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_SWIZZLE_A, tex_swizzle_a);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_SWIZZLE_R, tex_swizzle_r);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_SWIZZLE_G, tex_swizzle_g);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_SWIZZLE_B, tex_swizzle_b);
        }
        tex_swizzle_a = GL_ALPHA;
        tex_swizzle_r = GL_RED;
        tex_swizzle_g = GL_GREEN;
        tex_swizzle_b = GL_BLUE;
    };

    OpenGL::Texture cached_texture;
    
    // Check if the texture is a framebuffer
    if (cache.getFramebuffer(texture.addr, cached_texture)) {
        glActiveTexture(GL_TEXTURE0 + 0);
        glBindTexture(GL_TEXTURE_2D, cached_texture.m_handle);
        last_tex = texture;

        // We flip framebuffer textures because OpenGL renders to them upside down
        should_flip_tex = true;
        
        swizzle();
        return;
    }

    should_flip_tex = false;

    // Texture cache
    const u64 hash = cache.computeTextureHash(ps3->mem.getPtr(texture.addr), texture.width, texture.height, 4);    // TODO: don't hardcode
    if (!cache.getTexture(hash, cached_texture)) {
        const auto fmt = getTexturePixelFormat(texture.format);
        const auto internal = getTextureInternalFormat(texture.format);
        const auto type = getTextureDataType(texture.format);

        glGenTextures(1, &cached_texture.m_handle);
        glBindTexture(GL_TEXTURE_2D, cached_texture.m_handle);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_MIRRORED_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_MIRRORED_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glActiveTexture(GL_TEXTURE0 + 0);
        if (!isCompressedFormat(texture.format)) {
            glPixelStorei(GL_UNPACK_ROW_LENGTH, tex_pitch / ((getRawTextureFormat(texture.format) == CELL_GCM_TEXTURE_B8) ? 1 : 4)); // TODO: Clean this up

            u8* tex_ptr = ps3->mem.getPtr(texture.addr);
            u8* unswizzled_tex = nullptr;
            // Handle swizzling
            if ((texture.format & CELL_GCM_TEXTURE_LN) == CELL_GCM_TEXTURE_SZ) {
                const u32 pixel_size = (getRawTextureFormat(texture.format) == CELL_GCM_TEXTURE_B8) ? 1 : 4;    // TODO: Other formats
                unswizzled_tex = new u8[texture.width * texture.height * pixel_size];
                swizzleTexture(tex_ptr, unswizzled_tex, texture.width, texture.height, pixel_size);
            }

            glTexImage2D(GL_TEXTURE_2D, 0, internal, texture.width, texture.height, 0, fmt, type, (void*)(!unswizzled_tex ? tex_ptr : unswizzled_tex));
            //checkGLError();

            delete[] unswizzled_tex;
        }
        else {
            glCompressedTexImage2D(GL_TEXTURE_2D, 0, internal, texture.width, texture.height, 0, getCompressedTextureSize(texture.format, texture.width, texture.height), (void*)ps3->mem.getPtr(texture.addr));
        }
        cache.cacheTexture(hash, cached_texture);
        //lodepng::encode(std::format("./{:08x}.png", texture.addr).c_str(), ps3->mem.getPtr(texture.addr), texture.width, texture.height);
    }
    glActiveTexture(GL_TEXTURE0 + 0);
    glBindTexture(GL_TEXTURE_2D, cached_texture.m_handle);
    swizzle();

    last_tex = texture;
}

void RSX::swizzleTexture(u8* src, u8* dst, u32 width, u32 height, u32 pixel_size) {
    auto swizzle = [pixel_size](u32 x, u32 y, u32 z, u32 log2_width, u32 log2_height, u32 log2_depth) {
        u32 offs = 0;

        u32 shift_count = 0;
        while (log2_width | log2_height | log2_depth) {
            if (log2_width) {
                offs |= (x & 0x01) << shift_count;
                x >>= 1;
                shift_count++;
                log2_width--;
            }
            if (log2_height) {
                offs |= (y & 0x01) << shift_count;
                y >>= 1;
                shift_count++;
                log2_height--;
            }
            if (log2_depth) {
                offs |= (z & 0x01) << shift_count;
                z >>= 1;
                shift_count++;
                log2_depth--;
            }
        }

        return offs * pixel_size;
    };

    const u32 log2_width = std::log2(width);
    const u32 log2_height = std::log2(height);

    for (u32 y = 0; y < height; y++) {
        for (u32 x = 0; x < width; x++) {
            const u32 offs = swizzle(x, y, 0, log2_width, log2_height, 0);
            std::memcpy(&dst[y * width * pixel_size + x * pixel_size], &src[offs], pixel_size);
        }
    }
}

void RSX::bindBuffer() {
    const u32 surface_a_addr = offsetAndLocationToAddress(surface_a_offset, surface_a_location & 1);
    log("Surface A addr: 0x%08x\n", surface_a_addr);

    glActiveTexture(GL_TEXTURE0 + 20);
    OpenGL::Texture cached_texture;
    if (!cache.getFramebuffer(surface_a_addr, cached_texture)) {
        // Generate a new framebuffer texture if we don't have a cached one
        glGenTextures(1, &cached_texture.m_handle);
        glBindTexture(GL_TEXTURE_2D, cached_texture.m_handle);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_MIRRORED_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_MIRRORED_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, 1280, 720, 0, GL_BGRA, GL_UNSIGNED_INT_8_8_8_8_REV, (void*)0);
        cache.cacheFramebuffer(surface_a_addr, cached_texture);
        // Bind and clear it
        fb.bind(GL_FRAMEBUFFER);
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, cached_texture.m_handle, 0);
        OpenGL::clearAll();
        glActiveTexture(GL_TEXTURE0 + 0);
        return;
    }

    // Just bind it
    fb.bind(GL_FRAMEBUFFER);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, cached_texture.m_handle, 0);
    glActiveTexture(GL_TEXTURE0 + 0);
}

void RSX::setupForDrawing() {
    compileProgram();
    setupVAO();
    uploadTexture();
    uploadVertexConstants();
    uploadFragmentUniforms();
    bindBuffer();
    OpenGL::enableScissor();
    OpenGL::setScissor(scissor_x, 720 - (scissor_y + scissor_height), scissor_width, scissor_height);
}

GLuint RSX::getTextureInternalFormat(u8 fmt) {
    switch (getRawTextureFormat(fmt)) {

    case CELL_GCM_TEXTURE_B8:               return GL_RED;
    case CELL_GCM_TEXTURE_R5G6B5:           return GL_RGB565;
    case CELL_GCM_TEXTURE_A8R8G8B8:         return GL_RGBA;
    case CELL_GCM_TEXTURE_D8R8G8B8:         return GL_RGBA;
    case CELL_GCM_TEXTURE_COMPRESSED_DXT1:  return GL_COMPRESSED_RGBA_S3TC_DXT1_EXT;
    case CELL_GCM_TEXTURE_COMPRESSED_DXT23: return GL_COMPRESSED_RGBA_S3TC_DXT3_EXT;
    case CELL_GCM_TEXTURE_COMPRESSED_DXT45: return GL_COMPRESSED_RGBA_S3TC_DXT5_EXT;
    case CELL_GCM_TEXTURE_G8B8:             return GL_RG;

    default:
        Helpers::panic("Unimplemented texture format 0x%02x (0x%02x)\n", fmt, getRawTextureFormat(fmt));
    }
}

GLuint RSX::getTexturePixelFormat(u8 fmt) {
    if ((fmt & CELL_GCM_TEXTURE_LN) == CELL_GCM_TEXTURE_SZ)
        log("SWIZZLED TEXTURE!!!\n");
    switch (getRawTextureFormat(fmt)) {

    case CELL_GCM_TEXTURE_B8:               return GL_RED;
    case CELL_GCM_TEXTURE_R5G6B5:           return GL_RGB;
    case CELL_GCM_TEXTURE_A8R8G8B8:         return GL_BGRA;
    case CELL_GCM_TEXTURE_D8R8G8B8:         return GL_BGRA;
    case CELL_GCM_TEXTURE_COMPRESSED_DXT1:  return GL_RGBA;
    case CELL_GCM_TEXTURE_COMPRESSED_DXT23: return GL_RGBA;
    case CELL_GCM_TEXTURE_COMPRESSED_DXT45: return GL_RGBA;
    case CELL_GCM_TEXTURE_G8B8:             return GL_RG;   // Untested

    default:
        Helpers::panic("Unimplemented texture format 0x%02x (0x%02x)\n", fmt, getRawTextureFormat(fmt));
    }
}

GLuint RSX::getTextureDataType(u8 fmt) {
    switch (getRawTextureFormat(fmt)) {
        
    case CELL_GCM_TEXTURE_R5G6B5:           return GL_UNSIGNED_SHORT_5_6_5;
    case CELL_GCM_TEXTURE_A8R8G8B8:         return GL_UNSIGNED_INT_8_8_8_8_REV;

    default:
        return GL_UNSIGNED_BYTE;
    }
}

bool RSX::isCompressedFormat(u8 fmt) {
    switch (getRawTextureFormat(fmt)) {

    case CELL_GCM_TEXTURE_COMPRESSED_DXT1:  return true;
    case CELL_GCM_TEXTURE_COMPRESSED_DXT23: return true;
    case CELL_GCM_TEXTURE_COMPRESSED_DXT45: return true;

    default:
        return false;
    }
}

size_t RSX::getCompressedTextureSize(u8 fmt, u32 width, u32 height) {
    switch (getRawTextureFormat(fmt)) {

    case CELL_GCM_TEXTURE_COMPRESSED_DXT1:  return ((width + 3) / 4) * ((height + 3) / 4) * 8;
    case CELL_GCM_TEXTURE_COMPRESSED_DXT23: return ((width + 3) / 4) * ((height + 3) / 4) * 16;
    case CELL_GCM_TEXTURE_COMPRESSED_DXT45: return ((width + 3) / 4) * ((height + 3) / 4) * 16;

    default:
        Helpers::panic("Tried to get compressed texture size of unimplemented format 0x%08x\n", fmt & ~(CELL_GCM_TEXTURE_LN | CELL_GCM_TEXTURE_UN));
    }
}

GLuint RSX::getPrimitive(u32 prim) {
    switch (prim) {

    case CELL_GCM_PRIMITIVE_POINTS:         return GL_POINTS;
    case CELL_GCM_PRIMITIVE_LINES:          return GL_LINES;
    case CELL_GCM_PRIMITIVE_LINE_LOOP:      return GL_LINE_LOOP;
    case CELL_GCM_PRIMITIVE_LINE_STRIP:     return GL_LINE_STRIP;
    case CELL_GCM_PRIMITIVE_TRIANGLES:      return GL_TRIANGLES;
    case CELL_GCM_PRIMITIVE_TRIANGLE_STRIP: return GL_TRIANGLE_STRIP;
    case CELL_GCM_PRIMITIVE_TRIANGLE_FAN:   return GL_TRIANGLE_FAN;
    case CELL_GCM_PRIMITIVE_QUADS:          return GL_TRIANGLE_STRIP;
    case CELL_GCM_PRIMITIVE_QUAD_STRIP:     Helpers::panic("Unimplemented quad strip\n");
    case CELL_GCM_PRIMITIVE_POLYGON:        Helpers::panic("Unimplemented polygon\n");

    default:                                Helpers::panic("Invalid RSX primitive type %d\n", prim);
    }
}

GLuint RSX::getBlendEquation(u16 eq) {
    switch (eq) {
    
    case CELL_GCM_FUNC_ADD:                     return GL_FUNC_ADD;
    case CELL_GCM_MIN:                          return GL_MIN;
    case CELL_GCM_MAX:                          return GL_MAX;
    case CELL_GCM_FUNC_SUBTRACT:                return GL_FUNC_SUBTRACT;
    case CELL_GCM_FUNC_REVERSE_SUBTRACT_SIGNED:
    case CELL_GCM_FUNC_REVERSE_SUBTRACT:        return GL_FUNC_REVERSE_SUBTRACT;
    
    default:
        log("WARNING: Unimplemented blend equation 0x%04x\n", eq);
        return GL_FUNC_ADD;
    }
} 

GLuint RSX::getBlendFactor(u16 fact) {
    switch (fact) {

    case CELL_GCM_ZERO:                     return GL_ZERO;
    case CELL_GCM_ONE:                      return GL_ONE;
    case CELL_GCM_SRC_COLOR:                return GL_SRC_COLOR;
    case CELL_GCM_ONE_MINUS_SRC_COLOR:      return GL_ONE_MINUS_SRC_COLOR;
    case CELL_GCM_DST_COLOR:                return GL_DST_COLOR;
    case CELL_GCM_ONE_MINUS_DST_COLOR:      return GL_ONE_MINUS_DST_COLOR;
    case CELL_GCM_SRC_ALPHA:                return GL_SRC_ALPHA;
    case CELL_GCM_ONE_MINUS_SRC_ALPHA:      return GL_ONE_MINUS_SRC_ALPHA;
    case CELL_GCM_DST_ALPHA:                return GL_DST_ALPHA;
    case CELL_GCM_ONE_MINUS_DST_ALPHA:      return GL_ONE_MINUS_DST_ALPHA;
    case CELL_GCM_SRC_ALPHA_SATURATE:       return GL_SRC_ALPHA_SATURATE;
    case CELL_GCM_CONSTANT_COLOR:           return GL_CONSTANT_COLOR;
    case CELL_GCM_ONE_MINUS_CONSTANT_COLOR: return GL_ONE_MINUS_CONSTANT_COLOR;
    case CELL_GCM_CONSTANT_ALPHA:           return GL_CONSTANT_ALPHA;
    case CELL_GCM_ONE_MINUS_CONSTANT_ALPHA: return GL_ONE_MINUS_CONSTANT_ALPHA;

    default:
        log("WARNING: Unimplemented blend factor 0x%04x\n");
        return GL_ONE;
    }
}

void RSX::runCommandList(u64 put_addr) {
    log("Executing commands\n");
    log("get: 0x%08x, put: 0x%08x\n", (u32)gcm.ctrl->get, (u32)gcm.ctrl->put);

    // Used to detect hangs
    hanged = false;
    u32 last_jump_addr = 0;
    u32 last_jump_dst = 0;

    // Execute while get != put
    // We increment get as we fetch data from the FIFO
    while (gcm.ctrl->get != gcm.ctrl->put) {
        u32 cmd = fetch32();
        auto cmd_num = cmd & 0x3ffff;
        auto argc = (cmd >> 18) & 0x7ff;

        if (cmd & 0xa0030003) {
            if ((cmd & 0xe0000003) == 0x20000000) { // jump
                const u32 old_get = gcm.ctrl->get - 4;
                gcm.ctrl->get = cmd & 0x1ffffffc;
                log("Jump to 0x%08x\n", (u32)gcm.ctrl->get);

                // Detect hangs
                if (gcm.ctrl->get == last_jump_dst && old_get == last_jump_addr) {
                    log("RSX hanged, aborting...\n");
                    hanged = true;
                    //exit(0);
                    break;
                }
                last_jump_addr = old_get;
                last_jump_dst = gcm.ctrl->get;
                continue;
            }

            if ((cmd & 0xe0000003) == 0x00000001) { // jump
                const u32 old_get = gcm.ctrl->get - 4;
                gcm.ctrl->get = cmd & 0xfffffffc;
                log("Jump to 0x%08x\n", (u32)gcm.ctrl->get);
                
                // Detect hangs
                if (gcm.ctrl->get == last_jump_dst && old_get == last_jump_addr) {
                    log("RSX hanged, aborting...\n");
                    hanged = true;
                    exit(0);
                    break;
                }
                last_jump_addr = old_get;
                last_jump_dst = gcm.ctrl->get;
                continue;
            }

            if ((cmd & 0x00000003) == 0x00000002) { // call
                call_stack.push(gcm.ctrl->get);
                gcm.ctrl->get = cmd & 0xfffffffc;
                log("Call 0x%08x\n", (u32)gcm.ctrl->get);
                continue;
            }

            if ((cmd & 0xffff0003) == 0x00020000) { // return
                Helpers::debugAssert(call_stack.size(), "RSX: Tried to return but the call stack was empty\n");
                gcm.ctrl->get = call_stack.top();
                call_stack.pop();
                log("Return\n");
                return;
            }
        }

        std::deque<u32> args;
        for (int i = 0; i < argc; i++)
            args.push_back(fetch32());

        bool incrementing = !(cmd & 0x40000000);    // CELL_GCM_METHOD_FLAG_NON_INCREMENT
        do {
            if (command_names.contains(cmd_num) && cmd_num)
                log("0x%08x: %s\n", (u32)gcm.ctrl->get - 4, command_names[cmd_num].c_str());
            doCmd(cmd_num, args);
            if (incrementing) cmd_num += 4;
        } while (!args.empty());
    }
}

void RSX::doCmd(u32 cmd_num, std::deque<u32>& args) {
    switch (cmd_num) {

    case NV406E_SET_REFERENCE: {
        log("ref: 0x%08x\n", args[0]);
        gcm.ctrl->ref = args[0];
        args.pop_front();
        break;
    }

    case NV406E_SEMAPHORE_OFFSET:
    case NV4097_SET_SEMAPHORE_OFFSET: {
        semaphore_offset = args[0];
        args.pop_front();
        break;
    }

    case NV4097_BACK_END_WRITE_SEMAPHORE_RELEASE: {
        const u32 val = (args[0] & 0xff00ff00) | ((args[0] & 0xff) << 16) | ((args[0] >> 16) & 0xff);
        ps3->mem.write<u32>(gcm.label_addr + semaphore_offset, val);
        args.pop_front();
        break;
    }

    case NV406E_SEMAPHORE_RELEASE:
    case NV4097_TEXTURE_READ_SEMAPHORE_RELEASE: {
        ps3->mem.write<u32>(gcm.label_addr + semaphore_offset, args[0]);
        args.pop_front();
        break;
    }

    case NV406E_SEMAPHORE_ACQUIRE: {
        const auto sema = ps3->mem.read<u32>(gcm.label_addr + semaphore_offset);
        if (sema != args[0]) {
            Helpers::panic("Could not acquire semaphore\n");
        }
        args.pop_front();
        break;
    }

    case NV4097_SET_CONTEXT_DMA_COLOR_A: {
        surface_a_location = args[0];
        log("Surface A: location: 0x%08x\n", surface_a_location);
        args.pop_front();
        break;
    }

    case NV4097_SET_CONTEXT_DMA_REPORT: {
        dma_report = args[0];
        log("Context DMA report location: 0x%08x\n", dma_report);
        args.pop_front();
        break;
    }

    case NV4097_SET_SURFACE_CLIP_HORIZONTAL: {
        // TODO: low 16 bits
        surface_clip[0] = args[0] >> 16;
        surface_clip_dirty = true;
        log("Surface clip width: %d\n", surface_clip[0]);
        args.pop_front();
        break;
    }

    case NV4097_SET_SURFACE_CLIP_VERTICAL: {
        // TODO: low 16 bits
        surface_clip[1] = args[0] >> 16;
        surface_clip_dirty = true;
        log("Surface clip height: %d\n", surface_clip[1]);
        args.pop_front();
        break;
    }

    case NV4097_SET_SURFACE_FORMAT: {
        // TODO: Everything else
        surface_a_offset = args[2];
        log("Surface A: offset: 0x%08x\n", surface_a_offset);
        
        args.pop_front();
        args.pop_front();
        args.pop_front();
        args.pop_front();
        args.pop_front();
        args.pop_front();
        break;
    }

    case NV4097_SET_SURFACE_COLOR_TARGET: {
        color_target = args[0];
        log("Color target: 0x%02x\n", color_target);
        args.pop_front();
        break;
    }

    case NV4097_SET_ALPHA_TEST_ENABLE: {
        // TODO
        if (args[0]) {
            log("Enabled alpha test\n");
        }
        else {
            log("Disabled alpha test\n");
        }
        args.pop_front();
        break;
    }

    case NV4097_SET_BLEND_ENABLE: {
        if (args[0]) {
            log("Enabled blending\n");
            OpenGL::enableBlend();
        }
        else {
            log("Disabled blending\n");
            OpenGL::disableBlend();
        }
        args.pop_front();
        break;
    }

    case NV4097_SET_BLEND_FUNC_SFACTOR: {
        blend_sfactor_rgb = args[0] & 0xffff;
        blend_sfactor_a = args[0] >> 16;
        glBlendFuncSeparate(getBlendFactor(blend_sfactor_rgb), getBlendFactor(blend_dfactor_rgb), getBlendFactor(blend_sfactor_a), getBlendFactor(blend_dfactor_a));
        
        args.pop_front();
        break;
    }

    case NV4097_SET_BLEND_FUNC_DFACTOR: {
        blend_dfactor_rgb = args[0] & 0xffff;
        blend_dfactor_a = args[0] >> 16;
        glBlendFuncSeparate(getBlendFactor(blend_sfactor_rgb), getBlendFactor(blend_dfactor_rgb), getBlendFactor(blend_sfactor_a), getBlendFactor(blend_dfactor_a));

        args.pop_front();
        break;
    }

    case NV4097_SET_BLEND_COLOR: {
        blend_color_r = (args[0] >> 0) & 0xff;
        blend_color_g = (args[0] >> 8) & 0xff;
        blend_color_b = (args[0] >> 16) & 0xff;
        blend_color_a = (args[0] >> 24) & 0xff;

        glBlendColor(blend_color_r / 255.0f, blend_color_g / 255.0f, blend_color_b / 255.0f, blend_color_a / 255.0f);
        args.pop_front();
        break;
    }

    case NV4097_SET_BLEND_EQUATION: {
        blend_equation_rgb = args[0] & 0xffff;
        blend_equation_alpha = args[0] >> 16;

        glBlendEquationSeparate(getBlendEquation(blend_equation_rgb), getBlendEquation(blend_equation_alpha));
        args.pop_front();
        break;
    }

    case NV4097_SET_SCISSOR_HORIZONTAL: {
        scissor_x = args[0] & 0xffff;
        scissor_width = args[0] >> 16;
        args.pop_front();
        break;
    }

    case NV4097_SET_SCISSOR_VERTICAL: {
        scissor_y = args[0] & 0xffff;
        scissor_height = args[0] >> 16;
        args.pop_front();
        break;
    }

    case NV4097_SET_SHADER_PROGRAM: {
        fragment_shader_program.addr = offsetAndLocationToAddress(args[0] & ~3, (args[0] & 3) - 1);
        log("Fragment shader: address: 0x%08x\n", fragment_shader_program.addr);
        args.pop_front();
        break;
    }
    
    case NV4097_SET_VIEWPORT_OFFSET + 12:
    case NV4097_SET_VIEWPORT_OFFSET + 8:
    case NV4097_SET_VIEWPORT_OFFSET + 4:
    case NV4097_SET_VIEWPORT_OFFSET: {
        const auto idx = (cmd_num - NV4097_SET_VIEWPORT_OFFSET) / 4;
        viewport_offs[idx] = reinterpret_cast<float&>(args[0]);
        viewport_offs_dirty = true;
        log("Viewport offset %c: %f\n", idx == 0 ? 'x' : (idx == 1 ? 'y' : (idx == 2 ? 'z' : 'w')), viewport_offs[idx]);
        args.pop_front();
        break;
    }

    case NV4097_SET_VIEWPORT_SCALE + 12:
    case NV4097_SET_VIEWPORT_SCALE + 8:
    case NV4097_SET_VIEWPORT_SCALE + 4:
    case NV4097_SET_VIEWPORT_SCALE: {
        const auto idx = (cmd_num - NV4097_SET_VIEWPORT_SCALE) / 4;
        viewport_scale[idx] = reinterpret_cast<float&>(args[0]);
        viewport_scale_dirty = true;
        log("Viewport scale %c: %f\n", idx == 0 ? 'x' : (idx == 1 ? 'y' : (idx == 2 ? 'z' : 'w')), viewport_scale[idx]);
        args.pop_front();
        break;
    }

    case NV4097_SET_DEPTH_FUNC: {
        glDepthFunc(args[0]);
        args.pop_front();
        break;
    }
            
    case NV4097_SET_DEPTH_MASK: {
        depth_mask = args[0];
        
        if (args[0]) {
            log("Enabled depth mask\n");
            glDepthMask(GL_TRUE);
        }
        else {
            log("Disabled depth mask\n");
            glDepthMask(GL_FALSE);
        }
        args.pop_front();
        break;
    }
            
    case NV4097_SET_DEPTH_TEST_ENABLE: {
        if (args[0]) {
            log("Enabled depth test\n");
            OpenGL::enableDepth();
        }
        else {
            log("Disabled depth test\n");
            OpenGL::disableDepth();
        }
        args.pop_front();
        break;
    }

    case NV4097_SET_TRANSFORM_PROGRAM: {
        for (int i = 0; i < args.size(); i++)
            vertex_shader_data[vertex_shader_load_idx * 4 + i] = args[i];
        vertex_shader_load_idx += args.size() / 4;
        log("Vertex shader: uploading %d words (%d instructions)\n", args.size(), args.size() / 4);
        args.clear();
        break;
    }

    case NV4097_SET_VERTEX_DATA_ARRAY_OFFSET + 4:
    case NV4097_SET_VERTEX_DATA_ARRAY_OFFSET + 8:
    case NV4097_SET_VERTEX_DATA_ARRAY_OFFSET + 12:
    case NV4097_SET_VERTEX_DATA_ARRAY_OFFSET + 16:
    case NV4097_SET_VERTEX_DATA_ARRAY_OFFSET + 20:
    case NV4097_SET_VERTEX_DATA_ARRAY_OFFSET + 24:
    case NV4097_SET_VERTEX_DATA_ARRAY_OFFSET + 28:
    case NV4097_SET_VERTEX_DATA_ARRAY_OFFSET + 32:
    case NV4097_SET_VERTEX_DATA_ARRAY_OFFSET + 36:
    case NV4097_SET_VERTEX_DATA_ARRAY_OFFSET + 40:
    case NV4097_SET_VERTEX_DATA_ARRAY_OFFSET + 44:
    case NV4097_SET_VERTEX_DATA_ARRAY_OFFSET + 48:
    case NV4097_SET_VERTEX_DATA_ARRAY_OFFSET + 52:
    case NV4097_SET_VERTEX_DATA_ARRAY_OFFSET + 56:
    case NV4097_SET_VERTEX_DATA_ARRAY_OFFSET + 60:
    case NV4097_SET_VERTEX_DATA_ARRAY_OFFSET: {
        // size == 0 means binding is disabled
        const int idx = (cmd_num - NV4097_SET_VERTEX_DATA_ARRAY_OFFSET) >> 2;
        const u32 offset = args[0] & 0x7fffffff;
        const u8 location = args[0] >> 31;
        vertex_array.bindings[idx].offset = offsetAndLocationToAddress(offset, location);
        log("Vertex attribute %d: offset: 0x%08x\n", vertex_array.bindings[idx].index, vertex_array.bindings[idx].offset);
        args.pop_front();
        break;
    }

    case NV4097_SET_VERTEX_DATA_ARRAY_FORMAT + 4:
    case NV4097_SET_VERTEX_DATA_ARRAY_FORMAT + 8:
    case NV4097_SET_VERTEX_DATA_ARRAY_FORMAT + 12:
    case NV4097_SET_VERTEX_DATA_ARRAY_FORMAT + 16:
    case NV4097_SET_VERTEX_DATA_ARRAY_FORMAT + 20:
    case NV4097_SET_VERTEX_DATA_ARRAY_FORMAT + 24:
    case NV4097_SET_VERTEX_DATA_ARRAY_FORMAT + 28:
    case NV4097_SET_VERTEX_DATA_ARRAY_FORMAT + 32:
    case NV4097_SET_VERTEX_DATA_ARRAY_FORMAT + 36:
    case NV4097_SET_VERTEX_DATA_ARRAY_FORMAT + 40:
    case NV4097_SET_VERTEX_DATA_ARRAY_FORMAT + 44:
    case NV4097_SET_VERTEX_DATA_ARRAY_FORMAT + 48:
    case NV4097_SET_VERTEX_DATA_ARRAY_FORMAT + 52:
    case NV4097_SET_VERTEX_DATA_ARRAY_FORMAT + 56:
    case NV4097_SET_VERTEX_DATA_ARRAY_FORMAT + 60:
    case NV4097_SET_VERTEX_DATA_ARRAY_FORMAT: {
        const int idx = (cmd_num - NV4097_SET_VERTEX_DATA_ARRAY_FORMAT) >> 2;
        vertex_array.bindings[idx].index = idx;
        vertex_array.bindings[idx].type = args[0] & 0xf;
        vertex_array.bindings[idx].size = (args[0] >> 4) & 0xf;
        vertex_array.bindings[idx].stride = (args[0] >> 8) & 0xff;
        log("Vertex attribute %d: size: %d, stride: 0x%02x, type: %d\n", vertex_array.bindings[idx].index, vertex_array.bindings[idx].size, vertex_array.bindings[idx].stride, vertex_array.bindings[idx].type);
        args.pop_front();
        break;
    }

    case NV4097_SET_BEGIN_END: {
        const u32 prim = args[0];
        log("Primitive: 0x%0x\n", prim);

        if (prim == 0) {   // End
            //vertex_array.bindings.clear();

            // Immediate mode drawing
            int n_verts = 0;
            if (has_immediate_data) {
                // Construct vertex buffer from immediate data (this is slow)
                std::vector<u8> buffer;
                for (auto& binding : immediate_data.bindings) {
                    if (binding.n_verts > 0) {    // Binding is active
                        if (binding.n_verts > n_verts) n_verts = binding.n_verts;

                        const u32 old_size = buffer.size();
                        buffer.resize(old_size + binding.data.size());
                        std::memcpy(&buffer[old_size], binding.data.data(), binding.data.size());

                        // Setup VAO attribute
                        switch (binding.type) {
                        case 1:
                            vao.setAttributeFloat<GLshort>(binding.index, binding.size, binding.stride, (void*)old_size, true);
                            break;
                        case 2:
                            vao.setAttributeFloat<float>(binding.index, binding.size, binding.stride, (void*)old_size, false);
                            break;
                        case 4:
                            vao.setAttributeFloat<GLubyte>(binding.index, binding.size, binding.stride, (void*)old_size, true);
                            break;
                        case 5:
                            vao.setAttributeFloat<GLshort>(binding.index, binding.size, binding.stride, (void*)old_size, false);
                            break;
                        case 7:
                            vao.setAttributeFloat<GLubyte>(binding.index, binding.size, binding.stride, (void*)old_size, false);
                            break;
                        default:
                            Helpers::panic("Unimplemented vertex attribute type %d\n", binding.type);
                        }
                        vao.enableAttribute(binding.index);
                    }
                }

                // We don't use setupForDrawing() because we setup the VAO differently above. Can't use setupVAO()
                compileProgram();
                uploadTexture();
                uploadVertexConstants();
                uploadFragmentUniforms();
                bindBuffer();
                OpenGL::setScissor(scissor_x, 720 - (scissor_y + scissor_height), scissor_width, scissor_height);

                // Hack for quads
                if (primitive == CELL_GCM_PRIMITIVE_QUADS) {
                    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, quad_ibo);
                    quad_index_array.clear();
                    for (int i = 0; i < n_verts / 4; i++) {
                        if (i > 0) {
                            quad_index_array.push_back(quad_index_array.back());
                            quad_index_array.push_back((i * 4) + 0);
                        }
                        
                        quad_index_array.push_back((i * 4) + 0);
                        quad_index_array.push_back((i * 4) + 1);
                        quad_index_array.push_back((i * 4) + 3);
                        quad_index_array.push_back((i * 4) + 2);
                    }
                    glBufferData(GL_ELEMENT_ARRAY_BUFFER, quad_index_array.size() * 4, quad_index_array.data(), GL_STATIC_DRAW);
                    glBufferData(GL_ARRAY_BUFFER, buffer.size(), (void*)buffer.data(), GL_STATIC_DRAW);
                    glDrawElements(getPrimitive(primitive), quad_index_array.size(), GL_UNSIGNED_INT, 0);
                }
                else {
                    glBufferData(GL_ARRAY_BUFFER, buffer.size(), (void*)buffer.data(), GL_STATIC_DRAW);
                    glDrawArrays(getPrimitive(primitive), 0, n_verts);
                }

                has_immediate_data = false;
                for (auto& i : immediate_data.bindings) {
                    i.n_verts = 0;
                    i.data.clear();
                }
            }

            // Inlined array
            if (inline_array.size()) {
                setupForDrawing();

                // Find how many vertices worth of data we have
                u32 highest = 0;
                AttributeBinding* highest_binding = nullptr;
                for (auto& binding : vertex_array.bindings) {
                    if (!binding.size) continue;
                    if (binding.offset > highest) {
                        highest = binding.offset;
                        highest_binding = &binding;
                    }
                }

                if (!highest_binding) {
                    Helpers::panic("VERTEX ARRAY WITH NO ATTRIBUTE BINDINGS!\n");
                }

                const auto n_bytes = inline_array.size() * sizeof(u32);
                const auto attrib_size = highest_binding->sizeOfComponent() * highest_binding->size;
                u32 n_vertices = 0;
                for (u32 i = highest_binding->offset - vertex_array.getBase(); i + attrib_size <= n_bytes; i += highest_binding->stride)
                    n_vertices++;
                log("Drawing inline array: %d vertices\n", n_vertices);

                // Gather vertices and draw
                std::vector<u8> vtx_buf;
                getVertices<true>(n_vertices, vtx_buf, 0);

                // Hack for quads
                if (primitive == CELL_GCM_PRIMITIVE_QUADS) {
                    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, quad_ibo);
                    quad_index_array.clear();
                    for (int i = 0; i < n_vertices / 4; i++) {
                        if (i > 0) {
                            quad_index_array.push_back(quad_index_array.back());
                            quad_index_array.push_back((i * 4) + 0);
                        }
                        
                        quad_index_array.push_back((i * 4) + 0);
                        quad_index_array.push_back((i * 4) + 1);
                        quad_index_array.push_back((i * 4) + 3);
                        quad_index_array.push_back((i * 4) + 2);
                    }
                    glBufferData(GL_ELEMENT_ARRAY_BUFFER, quad_index_array.size() * 4, quad_index_array.data(), GL_STATIC_DRAW);
                    glBufferData(GL_ARRAY_BUFFER, vtx_buf.size(), (void*)vtx_buf.data(), GL_STATIC_DRAW);
                    glDrawElements(getPrimitive(primitive), quad_index_array.size(), GL_UNSIGNED_INT, 0);
                }
                else {
                    glBufferData(GL_ARRAY_BUFFER, vtx_buf.size(), (void*)vtx_buf.data(), GL_STATIC_DRAW);
                    glDrawArrays(getPrimitive(primitive), 0, n_vertices);
                }

                inline_array.clear();
            }
        }

        primitive = prim;
        args.pop_front();
        break;
    }

    case NV4097_DRAW_ARRAYS: {
        setupForDrawing();

        std::vector<u8> vtx_buf;
        int n_verts = 0;
        for (auto& j : args) {
            const u32 first = j & 0xffffff;
            const u32 count = (j >> 24) + 1;
            n_verts += count;

            log("Draw Arrays: first: %d count: %d\n", first, count);
            getVertices(count, vtx_buf, first);
        }

        // Hack for quads
        if (primitive == CELL_GCM_PRIMITIVE_QUADS) {
            glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, quad_ibo);
            quad_index_array.clear();
            for (int i = 0; i < n_verts / 4; i++) {
                if (i > 0) {
                    quad_index_array.push_back(quad_index_array.back());
                    quad_index_array.push_back((i * 4) + 0);
                }
                
                quad_index_array.push_back((i * 4) + 0);
                quad_index_array.push_back((i * 4) + 1);
                quad_index_array.push_back((i * 4) + 3);
                quad_index_array.push_back((i * 4) + 2);
            }
            glBufferData(GL_ELEMENT_ARRAY_BUFFER, quad_index_array.size() * 4, quad_index_array.data(), GL_STATIC_DRAW);
            glBufferData(GL_ARRAY_BUFFER, vtx_buf.size(), (void*)vtx_buf.data(), GL_STATIC_DRAW);
            glDrawElements(getPrimitive(primitive), quad_index_array.size(), GL_UNSIGNED_INT, 0);
        }
        else {
            glBufferData(GL_ARRAY_BUFFER, vtx_buf.size(), (void*)vtx_buf.data(), GL_STATIC_DRAW);
            glDrawArrays(getPrimitive(primitive), 0, n_verts);
        }

        args.clear();
        break;
    }

    case NV4097_INLINE_ARRAY: {
        log("Inline array: 0x%08x\n", args[0]);
        inline_array.push_back(args[0]);
        args.pop_front();
        break;
    }

    case NV4097_SET_INDEX_ARRAY_ADDRESS: {
        index_array.addr = args[0];
        log("Index array: offs: 0x%08x\n", index_array.addr);
        args.pop_front();
        break;
    }

    case NV4097_SET_INDEX_ARRAY_DMA: {
        const u32 location = args[0] & 0xf;   // Local or RSX
        const u32 addr = offsetAndLocationToAddress(index_array.addr, location);
        const u8 type = (args[0] >> 4) & 0xf;
        index_array.addr = addr;
        index_array.type = type;
        log("Index array: addr: 0x%08x, type: %d, location: %s\n", addr, type, location == 0 ? "RSX" : "MAIN");
        args.pop_front();
        break;
    }

    case NV4097_DRAW_INDEX_ARRAY: {
        setupForDrawing();

        std::vector<u32> indices;
        u32 highest_index = 0;

        for (auto& j : args) {
            const u32 first = j & 0xffffff;
            const u32 count = (j >> 24) + 1;
            log("Draw Index Array: first: %d count: %d\n", first, count);
            if (index_array.type == 1) {
                for (int i = first; i < first + count; i++) {
                    const u16 index = ps3->mem.read<u16>(index_array.addr + i * 2);
                    indices.push_back(index);
                    if (index > highest_index) highest_index = index;
                }
            }
            else {
                Helpers::panic("Index buffer type 0\n");
            }
        }

        const auto n_vertices = highest_index + 1;
        log("Vertex buffer: %d vertices\n", n_vertices);

        // Hack for quads
        if (primitive == CELL_GCM_PRIMITIVE_QUADS) {
            auto quad_indices = indices;
            indices.clear();
            
            for (int i = 0; i < quad_indices.size(); i += 4) {
                const u32 v0 = quad_indices[i + 0];
                const u32 v1 = quad_indices[i + 1];
                const u32 v2 = quad_indices[i + 2];
                const u32 v3 = quad_indices[i + 3];
                
                if (i > 0) {
                    indices.push_back(indices.back());
                    indices.push_back(v0);
                }
                
                indices.push_back(v0);
                indices.push_back(v1);
                indices.push_back(v3);
                indices.push_back(v2);
            }
        }
        
        // Draw
        std::vector<u8> vtx_buf;
        getVertices(n_vertices, vtx_buf);

        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ibo);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * 4, indices.data(), GL_STATIC_DRAW);
        glBufferData(GL_ARRAY_BUFFER, vtx_buf.size(), (void*)vtx_buf.data(), GL_STATIC_DRAW);
        glDrawElements(getPrimitive(primitive), indices.size(), GL_UNSIGNED_INT, 0);

        args.clear();
        break;
    }
            
    case NV4097_SET_CULL_FACE_ENABLE: {
        if (args[0]) {
            log("Enabled cull face\n");
            //glEnable(GL_CULL_FACE);
            glCullFace(GL_BACK);
        } else {
            log("Disabled cull face\n");
            glDisable(GL_CULL_FACE);
        }
        
        args.pop_front();
        break;
    }

    case NV4097_SET_TEXTURE_CONTROL3: {
        tex_pitch = args[0] & 0xfffff;
        args.pop_front();
        break;
    }

    case NV4097_SET_VERTEX_DATA2F_M + 8:
    case NV4097_SET_VERTEX_DATA2F_M + 16:
    case NV4097_SET_VERTEX_DATA2F_M + 24:
    case NV4097_SET_VERTEX_DATA2F_M + 32:
    case NV4097_SET_VERTEX_DATA2F_M + 40:
    case NV4097_SET_VERTEX_DATA2F_M + 48:
    case NV4097_SET_VERTEX_DATA2F_M + 56:
    case NV4097_SET_VERTEX_DATA2F_M + 64:
    case NV4097_SET_VERTEX_DATA2F_M + 72:
    case NV4097_SET_VERTEX_DATA2F_M + 80:
    case NV4097_SET_VERTEX_DATA2F_M + 88:
    case NV4097_SET_VERTEX_DATA2F_M + 96:
    case NV4097_SET_VERTEX_DATA2F_M + 104:
    case NV4097_SET_VERTEX_DATA2F_M + 112:
    case NV4097_SET_VERTEX_DATA2F_M: {
        const u32 idx = (cmd_num - NV4097_SET_VERTEX_DATA2F_M) >> 3;
        const float x = reinterpret_cast<float&>(args[0]);
        const float y = reinterpret_cast<float&>(args[1]);
        log("Attribute %d: {%f, %f}\n", idx, x, y);

        // TODO: Should probably check if it tries to upload different types of data to the same binding.
        // That's not supposed to happen
        immediate_data.bindings[idx].index = idx;
        immediate_data.bindings[idx].type = 2;  // Float
        immediate_data.bindings[idx].size = 2;  // Elements per vertex
        immediate_data.bindings[idx].stride = 2 * sizeof(float);    // Stride
        immediate_data.bindings[idx].n_verts++;
        const u32 old_size = immediate_data.bindings[idx].data.size();
        // Append new data to vector
        immediate_data.bindings[idx].data.resize(old_size + sizeof(float) * 2);
        reinterpret_cast<float&>(immediate_data.bindings[idx].data[old_size + 0 * sizeof(float)]) = x;
        reinterpret_cast<float&>(immediate_data.bindings[idx].data[old_size + 1 * sizeof(float)]) = y;
        // Set a flag indicating that immediate data has been uploaded
        has_immediate_data = true;

        args.pop_front();
        args.pop_front();
        break;
    }

    case NV4097_SET_TEXTURE_OFFSET: {
        const u32 offs = args[0];
        log("Set texture: offset: 0x%08x\n", offs);
        texture.offs = offs;
        texture.addr = offsetAndLocationToAddress(texture.offs, texture.loc);

        args.pop_front();
        break;
    }

    case NV4097_SET_TEXTURE_FORMAT: {
        const u8 loc = (args[0] & 0x3) - 1;
        const u32 addr = offsetAndLocationToAddress(texture.offs, loc);
        const u8 dimension = (args[0] >> 4) & 0xf;
        const u8 format = (args[0] >> 8) & 0xff;
        // TODO: mipmap, cubemap
        log("Set texture: addr: 0x%08x, dimension: 0x%02x, format: 0x%02x, location: %s\n", addr, dimension, format, loc == 0 ? "RSX" : "MAIN");

        texture.loc = loc;
        texture.addr = addr;
        texture.format = format;
        
        args.pop_front();
        break;
    }

    case NV4097_SET_TEXTURE_ADDRESS: {
        log("NV4097_SET_TEXTURE_ADDRESS\n");
        args.pop_front();
        break;
    }

    case NV4097_SET_TEXTURE_CONTROL0: {
        log("NV4097_SET_TEXTURE_CONTROL0\n");
        args.pop_front();
        break;
    }

    case NV4097_SET_TEXTURE_CONTROL1: {
        control1 = args[0];
        args.pop_front();
        break;
    }

    case NV4097_SET_TEXTURE_FILTER: {
        log("NV4097_SET_TEXTURE_FILTER\n");
        args.pop_front();
        break;
    }

    case NV4097_SET_TEXTURE_IMAGE_RECT: {
        const u16 width = args[0] >> 16;
        const u16 height = args[0] & 0xfffff;
        log("Texture: width: %d, height: %d\n", width, height);

        texture.width = width;
        texture.height = height;

        args.pop_front();
        break;
    }

    case NV4097_SET_VERTEX_DATA4F_M + 16:
    case NV4097_SET_VERTEX_DATA4F_M + 32:
    case NV4097_SET_VERTEX_DATA4F_M + 48:
    case NV4097_SET_VERTEX_DATA4F_M + 64:
    case NV4097_SET_VERTEX_DATA4F_M + 80:
    case NV4097_SET_VERTEX_DATA4F_M + 96:
    case NV4097_SET_VERTEX_DATA4F_M + 112:
    case NV4097_SET_VERTEX_DATA4F_M + 128:
    case NV4097_SET_VERTEX_DATA4F_M + 144:
    case NV4097_SET_VERTEX_DATA4F_M + 160:
    case NV4097_SET_VERTEX_DATA4F_M + 176:
    case NV4097_SET_VERTEX_DATA4F_M + 192:
    case NV4097_SET_VERTEX_DATA4F_M + 208:
    case NV4097_SET_VERTEX_DATA4F_M + 224:
    case NV4097_SET_VERTEX_DATA4F_M: {
        const u32 idx = (cmd_num - NV4097_SET_VERTEX_DATA4F_M) >> 4;
        const float x = reinterpret_cast<float&>(args[0]);
        const float y = reinterpret_cast<float&>(args[1]);
        const float z = reinterpret_cast<float&>(args[2]);
        const float w = reinterpret_cast<float&>(args[3]);
        log("Attribute %d: {%f, %f, %f, %f}\n", idx, x, y, z, w);

        // TODO: Should probably check if it tries to upload different types of data to the same binding.
        // That's not supposed to happen
        immediate_data.bindings[idx].index = idx;
        immediate_data.bindings[idx].type = 2;  // Float
        immediate_data.bindings[idx].size = 4;  // Elements per vertex
        immediate_data.bindings[idx].stride = 4 * sizeof(float);    // Stride
        immediate_data.bindings[idx].n_verts++;
        const u32 old_size = immediate_data.bindings[idx].data.size();
        // Append new data to vector
        immediate_data.bindings[idx].data.resize(old_size + sizeof(float) * 4);
        reinterpret_cast<float&>(immediate_data.bindings[idx].data[old_size + 0 * sizeof(float)]) = x;
        reinterpret_cast<float&>(immediate_data.bindings[idx].data[old_size + 1 * sizeof(float)]) = y;
        reinterpret_cast<float&>(immediate_data.bindings[idx].data[old_size + 2 * sizeof(float)]) = z;
        reinterpret_cast<float&>(immediate_data.bindings[idx].data[old_size + 3 * sizeof(float)]) = w;
        // Set a flag indicating that immediate data has been uploaded
        has_immediate_data = true;

        args.pop_front();
        args.pop_front();
        args.pop_front();
        args.pop_front();
        break;
    }

    case NV4097_SET_SHADER_CONTROL: {
        fragment_shader_program.ctrl = args[0];
        log("Fragment shader: control: 0x%08x\n", fragment_shader_program.ctrl);

        args.pop_front();
        break;
    }

    case NV4097_SET_COLOR_CLEAR_VALUE: {
        clear_color.r() = ((args[0] >> 0) & 0xff) / 255.0f;
        clear_color.g() = ((args[0] >> 8) & 0xff) / 255.0f;
        clear_color.b() = ((args[0] >> 16) & 0xff) / 255.0f;
        clear_color.a() = ((args[0] >> 24) & 0xff) / 255.0f;

        args.pop_front();
        break;
    }

    case NV4097_CLEAR_SURFACE: {
        bindBuffer();
        OpenGL::setClearColor(clear_color.r(), clear_color.g(), clear_color.b(), clear_color.a());
        if (args[0] & 0xf0)
            OpenGL::clearColor();
        if (args[0] & 1) {
            glDepthMask(GL_TRUE);
            OpenGL::clearDepth();
            glDepthMask(depth_mask ? GL_TRUE : GL_FALSE);
        }
        if (args[0] & 2)
            OpenGL::clearStencil();

        args.pop_front();
        break;
    }

    case NV4097_SET_TRANSFORM_PROGRAM_LOAD: {
        // This is the instruction index NV4097_SET_TRANSFORM_PROGRAM will begin loading the instructions at
        vertex_shader_load_idx = args[0];
        log("Vertex shader load: %d\n", vertex_shader_load_idx);

        args.pop_front();
        break;
    }

    case NV4097_SET_TRANSFORM_PROGRAM_START: {
        // This is the index of the first vertex shader instruction
        vertex_shader_start_idx = args[0];
        log("Vertex shader start: %d\n", vertex_shader_start_idx);

        args.pop_front();
        break;
    }

    case NV4097_SET_TRANSFORM_CONSTANT_LOAD: {
        const u32 start = args[0];
        for (int i = 1; i < args.size(); i++) constants[start * 4 + i - 1] = args[i];
        constants_dirty = true;

        log("Upload %d transform constants starting at %d\n", args.size() - 1, args[0]);
        for (int i = 1; i < args.size(); i++) {
            log("0x%08x (%f)\n", constants[start * 4 + i - 1], reinterpret_cast<float&>(constants[start * 4 + i - 1]));
        }

        args.clear();
        break;
    }

    case NV4097_SET_VERTEX_ATTRIB_OUTPUT_MASK: {
        for (int i = 0; i < 22; i++) {
            if (args[0] & (1 << i)) {
                fragment_shader_decompiler.enableInput(i);
            }
        }

        args.pop_front();
        break;
    }

    case NV3062_SET_OFFSET_DESTIN: {
        dest_offset = args[0];
        log("Dest offset: 0x%08x\n", dest_offset);
        args.pop_front();
        break;
    }

    case NV308A_POINT: {
        point_x = args[0] & 0xffff;
        point_y = args[0] >> 16;
        log("Point: { x: 0x%04x, y: 0x%04x }\n", point_x, point_y);
        args.pop_front();
        break;
    }

    case NV308A_COLOR: {
        u32 addr = offsetAndLocationToAddress(dest_offset + (point_x << 2), 0);
        Helpers::debugAssert(args.size() <= 4, "NV308A_COLOR: args size > 4\n");
        float v[4] = { 0 };
        log("Color: addr: 0x%08x\n", addr);
        for (int i = 0; i < args.size(); i++) {
            u32 swapped = (args[i] >> 16) | (args[i] << 16);
            v[i] = reinterpret_cast<float&>(swapped);
            log("Uploaded float 0x%08x\n", args[i]);
        }
        const auto name = fragment_shader_decompiler.addUniform(addr);
        fragment_uniforms.push_back({ name, v[0], v[1], v[2], v[3] });

        args.clear();
        break;
    }

    case GCM_FLIP_COMMAND: {
        const u32 buf_id = args[0];
        log("Flip %d\n", buf_id);

        // Blit to output framebuffer
        glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);
        glBlitFramebuffer(
            0, 0, 1280, 720,
            0, 0, 1280, 720,
            GL_COLOR_BUFFER_BIT,
            GL_NEAREST
        );
        
        // Reset state
        OpenGL::disableScissor();

        // Probably not right
        last_flip_time = std::chrono::system_clock::now().time_since_epoch().count() * 8;

        ps3->flip();
        args.pop_front();
        break;
    }
                         
    default: {
        // For unimplemented commands, clear the arguments
        // Skips any command following this if the unimplemented command is a command with increment
        args.clear();
        break;
    }

        /*
    default:
        if (cmd) {  // Don't print NOPs
            if (command_names.contains(cmd & 0x3ffff))
                log("Unimplemented RSX command %s\n", command_names[cmd_num].c_str());
            else
                log("Unimplemented RSX command 0x%08x (0x%08x)\n", cmd_num, cmd);
        }
        */
    }
}

void RSX::checkGLError() {
    GLenum err;
    while ((err = glGetError()) != GL_NO_ERROR) {
        Helpers::panic("GL error 0x%x\n", err);
    }
}
