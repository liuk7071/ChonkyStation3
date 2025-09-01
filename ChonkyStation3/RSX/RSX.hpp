#pragma once

#include <common.hpp>
#include <logger.hpp>
#include <opengl.hpp>

#include <unordered_map>
#include <stack>
#include <deque>
#include <chrono>

#include <VertexShaderDecompiler.hpp>
#include <FragmentShaderDecompiler.hpp>
#include <FragmentShader.hpp>
#include <RSXCache.hpp>
#include <Modules/CellGcmSys.hpp>


class PlayStation3;

class RSX {
public:
    RSX(PlayStation3* ps3);
    void initGL();  // TODO: decouple the OpenGL backend from the RSX core
    
    CellGcmSys& gcm;
    VertexShaderDecompiler vertex_shader_decompiler;
    FragmentShaderDecompiler fragment_shader_decompiler;
    RSXCache cache;

    PlayStation3* ps3;
    MAKE_LOG_FUNCTION(log, rsx);

    enum CellGcmTexture : u32 {
        // Color Flag
        CELL_GCM_TEXTURE_B8 = 0x81,
        CELL_GCM_TEXTURE_A1R5G5B5 = 0x82,
        CELL_GCM_TEXTURE_A4R4G4B4 = 0x83,
        CELL_GCM_TEXTURE_R5G6B5 = 0x84,
        CELL_GCM_TEXTURE_A8R8G8B8 = 0x85,
        CELL_GCM_TEXTURE_COMPRESSED_DXT1 = 0x86,
        CELL_GCM_TEXTURE_COMPRESSED_DXT23 = 0x87,
        CELL_GCM_TEXTURE_COMPRESSED_DXT45 = 0x88,
        CELL_GCM_TEXTURE_G8B8 = 0x8B,
        CELL_GCM_TEXTURE_COMPRESSED_B8R8_G8R8 = 0x8D,
        CELL_GCM_TEXTURE_COMPRESSED_R8B8_R8G8 = 0x8E,
        CELL_GCM_TEXTURE_R6G5B5 = 0x8F,
        CELL_GCM_TEXTURE_DEPTH24_D8 = 0x90,
        CELL_GCM_TEXTURE_DEPTH24_D8_FLOAT = 0x91,
        CELL_GCM_TEXTURE_DEPTH16 = 0x92,
        CELL_GCM_TEXTURE_DEPTH16_FLOAT = 0x93,
        CELL_GCM_TEXTURE_X16 = 0x94,
        CELL_GCM_TEXTURE_Y16_X16 = 0x95,
        CELL_GCM_TEXTURE_R5G5B5A1 = 0x97,
        CELL_GCM_TEXTURE_COMPRESSED_HILO8 = 0x98,
        CELL_GCM_TEXTURE_COMPRESSED_HILO_S8 = 0x99,
        CELL_GCM_TEXTURE_W16_Z16_Y16_X16_FLOAT = 0x9A,
        CELL_GCM_TEXTURE_W32_Z32_Y32_X32_FLOAT = 0x9B,
        CELL_GCM_TEXTURE_X32_FLOAT = 0x9C,
        CELL_GCM_TEXTURE_D1R5G5B5 = 0x9D,
        CELL_GCM_TEXTURE_D8R8G8B8 = 0x9E,
        CELL_GCM_TEXTURE_Y16_X16_FLOAT = 0x9F,

        // Swizzle Flag
        CELL_GCM_TEXTURE_SZ = 0x00,
        CELL_GCM_TEXTURE_LN = 0x20,

        // Normalization Flag
        CELL_GCM_TEXTURE_NR = 0x00,
        CELL_GCM_TEXTURE_UN = 0x40,
    };

    enum CellGcmPrimitive : u32 {
        CELL_GCM_PRIMITIVE_POINTS = 1,
        CELL_GCM_PRIMITIVE_LINES = 2,
        CELL_GCM_PRIMITIVE_LINE_LOOP = 3,
        CELL_GCM_PRIMITIVE_LINE_STRIP = 4,
        CELL_GCM_PRIMITIVE_TRIANGLES = 5,
        CELL_GCM_PRIMITIVE_TRIANGLE_STRIP = 6,
        CELL_GCM_PRIMITIVE_TRIANGLE_FAN = 7,
        CELL_GCM_PRIMITIVE_QUADS = 8,
        CELL_GCM_PRIMITIVE_QUAD_STRIP = 9,
        CELL_GCM_PRIMITIVE_POLYGON = 10,
    };

    enum CellGcmBlendEquation {
        CELL_GCM_FUNC_ADD = 0x8006,
        CELL_GCM_MIN = 0x8007,
        CELL_GCM_MAX = 0x8008,
        CELL_GCM_FUNC_SUBTRACT = 0x800A,
        CELL_GCM_FUNC_REVERSE_SUBTRACT = 0x800B,
        CELL_GCM_FUNC_REVERSE_SUBTRACT_SIGNED = 0x0000F005,
        CELL_GCM_FUNC_ADD_SIGNED = 0x0000F006,
        CELL_GCM_FUNC_REVERSE_ADD_SIGNED = 0x0000F007
    };

    enum CellGcmBlendFactor {
        CELL_GCM_ZERO = 0x0000,
        CELL_GCM_ONE = 0x0001,
        CELL_GCM_SRC_COLOR = 0x0300,
        CELL_GCM_ONE_MINUS_SRC_COLOR = 0x0301,
        CELL_GCM_SRC_ALPHA = 0x0302,
        CELL_GCM_ONE_MINUS_SRC_ALPHA = 0x0303,
        CELL_GCM_DST_ALPHA = 0x0304,
        CELL_GCM_ONE_MINUS_DST_ALPHA = 0x0305,
        CELL_GCM_DST_COLOR = 0x0306,
        CELL_GCM_ONE_MINUS_DST_COLOR = 0x0307,
        CELL_GCM_SRC_ALPHA_SATURATE = 0x0308,
        CELL_GCM_CONSTANT_COLOR = 0x8001,
        CELL_GCM_ONE_MINUS_CONSTANT_COLOR = 0x8002,
        CELL_GCM_CONSTANT_ALPHA = 0x8003,
        CELL_GCM_ONE_MINUS_CONSTANT_ALPHA = 0x8004,
    };

    u32 ea_table = 0;
    void setEaTableAddr(u32 addr);
    u32 ioToEa(u32 offs);

    void putWritten(u64 unused);
    void runCommandList();
    void doCmd(u32 cmd_num, std::deque<u32>& args);
    u32 fetch32();
    u32 offsetAndLocationToAddress(u32 offset, u8 location);

    std::stack<u32> call_stack;
    bool hanged = false;
    bool flipped = false;
    s64 last_flip_time = 0;

    OpenGL::Vector<float, 4> clear_color;
    u32 vertex_shader_data [512 * 4];   // 512 instructions, 1 qword each
    u32 vertex_shader_load_idx = 0;
    u32 vertex_shader_start_idx = 0;
    FragmentShader fragment_shader_program;
    std::vector<u32> quad_index_array;

    u32* constants = new u32[468 * 4]; // 468 * sizeof(vec4) / sizeof(float)
    bool constants_dirty = true;
    
    u64 last_program_hash = 0;
    bool program_changed = false;
    bool has_drawn_this_frame = false;
    
    struct FragmentUniform {
        std::string name;
        float x;
        float y;
        float z;
        float w;
    };
    std::vector<FragmentUniform> fragment_uniforms;

    struct Texture {
        u32 addr;
        u32 offs;
        u32 loc;
        u8 format;
        u16 width;
        u16 height;
        u32 control1 = 0;
        u32 tex_pitch = 0;

        bool operator==(const Texture& other) const {
            return addr == other.addr && format == other.format && width == other.width && height == other.height;
        }
    };
    Texture textures[16];
    Texture last_textures[16];

    u32 semaphore_offset = 0;
    u32 dest_offset = 0;
    u16 point_x = 0;
    u16 point_y = 0;
    u32 dma_report = 0;
    u32 primitive = 0;
    u16 blend_sfactor_rgb = CELL_GCM_ONE;
    u16 blend_sfactor_a = CELL_GCM_ONE;
    u16 blend_dfactor_rgb = CELL_GCM_ONE;
    u16 blend_dfactor_a = CELL_GCM_ONE;
    u8 blend_color_r = 0;
    u8 blend_color_g = 0;
    u8 blend_color_b = 0;
    u8 blend_color_a = 0;
    u16 blend_equation_rgb = 0;
    u16 blend_equation_alpha = 0;
    u32 vertex_shader_load_addr = 0;
    u32 color_target = 0;
    u32 surface_a_offset = 0;
    u32 surface_a_location = 0;
    u32 scissor_x = 0;
    u32 scissor_y = 0;
    u32 scissor_width = 1280;
    u32 scissor_height = 720;
    float viewport_offs[4];
    float viewport_scale[4];
    s32 surface_clip[2];
    bool viewport_offs_dirty = true;
    bool viewport_scale_dirty = true;
    bool surface_clip_dirty = true;
    bool depth_mask = true;

    static constexpr GLuint swizzle_map[] = {
        GL_ALPHA,
        GL_RED,
        GL_GREEN,
        GL_BLUE,
    };

    static constexpr char swizzle_str_map[] = {
        'a',
        'r',
        'g',
        'b'
    };

    GLuint tex_swizzle_a = GL_ALPHA;
    GLuint tex_swizzle_r = GL_RED;
    GLuint tex_swizzle_g = GL_GREEN;
    GLuint tex_swizzle_b = GL_BLUE;
    bool should_flip_textures[16];

    // Immediate vertex data (uploaded via the SET_VERTEX_DATA_* set of commands) can be use for OpenGL immediate-mode style drawing,
    // but it can also be mixed with normal drawing that uses vertex arrays.
    // For example, you can enable vertex attributes 0 and 1 in the vertex array, and then upload attribute 2 via immediate data.
    // Immediate data is meant for immediate drawing only if the data is uploaded inside a BEGIN/END block.
    // The flag below is set when that is the case.
    bool has_immediate_data = false;

    OpenGL::VertexArray vao;
    OpenGL::VertexBuffer vbo;
    OpenGL::Shader vertex, fragment;
    OpenGL::Program program;
    OpenGL::Texture tex;
    OpenGL::Framebuffer fb;
    OpenGL::Texture depth_tex;
    GLuint vertex_consts_ubo;

    GLuint ibo;
    GLuint quad_ibo;

    void checkGLError();

    struct AttributeBinding {
        u8 index;
        u8 stride;
        u8 size = 0;
        u32 offset;
        u8 type;

        // Only used in immediate drawing for now
        int n_verts = 0;    // Amount of vertices uploaded
        std::vector<u8> data;

        size_t sizeOfComponent() {
            size_t size;
            switch (type) {
            case 6:
            case 2: size = sizeof(float);   break;
            case 7:
            case 4: size = sizeof(u8);      break;
            case 1:
            case 3:
            case 5: size = sizeof(s16);     break;
            default:
                Helpers::panic("Tried to get size of unimplemented vertex type %d\n", type);
            }
            return size;
        }
    };

    class ImmediateDataArray {
    public:
        AttributeBinding bindings[15];
    };
    ImmediateDataArray immediate_data;

    class VertexArray {
    public:
        std::vector<AttributeBinding> bindings;
        
        u32 getBase() {
            u32 lowest = -1;
            for (auto& binding : bindings) {
                if (!binding.size) continue;
                if (binding.offset < lowest)
                    lowest = binding.offset;
            }
            return lowest;
        }

        // Returns size of 1 vertex
        u32 size() {
            u32 highest = 0;
            AttributeBinding* highest_binding = nullptr;
            for (auto& binding : bindings) {
                if (!binding.size) continue;
                if (binding.offset > highest) {
                    highest = binding.offset;
                    highest_binding = &binding;
                }
            }

            if (!highest_binding) {
                Helpers::panic("VERTEX ARRAY WITH NO ATTRIBUTE BINDINGS!\n");
                return 0;
            }
            return highest_binding->offset - getBase() + highest_binding->stride;
        }
    };
    VertexArray vertex_array;
    AttributeBinding curr_binding;
    std::vector<u32> inline_array;

    struct IndexArray {
        u32 addr;
        u8 type;
    };
    IndexArray index_array;

    void compileProgram();
    void setupVAO();
    template<bool is_inline_array = false> void getVertices(u32 n_vertices, std::vector<u8>& vtx_buf, u32 start = 0);
    void uploadVertexConstants();
    void uploadFragmentUniforms();
    void uploadTexture();
    void swizzleTexture(u8* src, u8* dst, u32 width, u32 height, u32 pixel_size);
    void bindBuffer();
    void setupForDrawing();

    u32 getRawTextureFormat(u8 fmt) { return fmt & ~(CELL_GCM_TEXTURE_LN | CELL_GCM_TEXTURE_UN); }
    GLuint getTexturePixelFormat(u8 fmt);
    GLuint getTextureInternalFormat(u8 fmt);
    GLuint getTextureDataType(u8 fmt);
    bool isCompressedFormat(u8 fmt);
    size_t getCompressedTextureSize(u8 fmt, u32 width, u32 height);
    GLuint getPrimitive(u32 prim);
    GLuint getBlendEquation(u16 eq);
    GLuint getBlendFactor(u16 fact);

    // I == command is implemented or at least handled in some way
    enum Commands : u32 {
        // NV406E
        NV406E_SET_REFERENCE                                    = 0x00000050,   // I
        NV406E_SET_CONTEXT_DMA_SEMAPHORE                        = 0x00000060,
        NV406E_SEMAPHORE_OFFSET                                 = 0x00000064,   // I
        NV406E_SEMAPHORE_ACQUIRE                                = 0x00000068,   // I
        NV406E_SEMAPHORE_RELEASE                                = 0x0000006c,   // I

        // NV4097
        NV4097_SET_OBJECT                                       = 0x00000000,
        NV4097_NO_OPERATION                                     = 0x00000100,
        NV4097_NOTIFY                                           = 0x00000104,
        NV4097_WAIT_FOR_IDLE                                    = 0x00000110,
        NV4097_PM_TRIGGER                                       = 0x00000140,
        NV4097_SET_CONTEXT_DMA_NOTIFIES                         = 0x00000180,
        NV4097_SET_CONTEXT_DMA_A                                = 0x00000184,
        NV4097_SET_CONTEXT_DMA_B                                = 0x00000188,
        NV4097_SET_CONTEXT_DMA_COLOR_B                          = 0x0000018c,
        NV4097_SET_CONTEXT_DMA_STATE                            = 0x00000190,
        NV4097_SET_CONTEXT_DMA_COLOR_A                          = 0x00000194,   // I
        NV4097_SET_CONTEXT_DMA_ZETA                             = 0x00000198,
        NV4097_SET_CONTEXT_DMA_VERTEX_A                         = 0x0000019c,
        NV4097_SET_CONTEXT_DMA_VERTEX_B                         = 0x000001a0,
        NV4097_SET_CONTEXT_DMA_SEMAPHORE                        = 0x000001a4,
        NV4097_SET_CONTEXT_DMA_REPORT                           = 0x000001a8,   // I
        NV4097_SET_CONTEXT_DMA_CLIP_ID                          = 0x000001ac,
        NV4097_SET_CONTEXT_DMA_CULL_DATA                        = 0x000001b0,
        NV4097_SET_CONTEXT_DMA_COLOR_C                          = 0x000001b4,
        NV4097_SET_CONTEXT_DMA_COLOR_D                          = 0x000001b8,
        NV4097_SET_SURFACE_CLIP_HORIZONTAL                      = 0x00000200,   // I
        NV4097_SET_SURFACE_CLIP_VERTICAL                        = 0x00000204,   // I
        NV4097_SET_SURFACE_FORMAT                               = 0x00000208,   // I
        NV4097_SET_SURFACE_PITCH_A                              = 0x0000020c,
        NV4097_SET_SURFACE_COLOR_AOFFSET                        = 0x00000210,
        NV4097_SET_SURFACE_ZETA_OFFSET                          = 0x00000214,
        NV4097_SET_SURFACE_COLOR_BOFFSET                        = 0x00000218,
        NV4097_SET_SURFACE_PITCH_B                              = 0x0000021c,
        NV4097_SET_SURFACE_COLOR_TARGET                         = 0x00000220,   // I
        NV4097_SET_SURFACE_PITCH_Z                              = 0x0000022c,
        NV4097_INVALIDATE_ZCULL                                 = 0x00000234,
        NV4097_SET_CYLINDRICAL_WRAP                             = 0x00000238,
        NV4097_SET_CYLINDRICAL_WRAP1                            = 0x0000023c,
        NV4097_SET_SURFACE_PITCH_C                              = 0x00000280,
        NV4097_SET_SURFACE_PITCH_D                              = 0x00000284,
        NV4097_SET_SURFACE_COLOR_COFFSET                        = 0x00000288,
        NV4097_SET_SURFACE_COLOR_DOFFSET                        = 0x0000028c,
        NV4097_SET_WINDOW_OFFSET                                = 0x000002b8,
        NV4097_SET_WINDOW_CLIP_TYPE                             = 0x000002bc,
        NV4097_SET_WINDOW_CLIP_HORIZONTAL                       = 0x000002c0,
        NV4097_SET_WINDOW_CLIP_VERTICAL                         = 0x000002c4,
        NV4097_SET_DITHER_ENABLE                                = 0x00000300,
        NV4097_SET_ALPHA_TEST_ENABLE                            = 0x00000304,   // I
        NV4097_SET_ALPHA_FUNC                                   = 0x00000308,
        NV4097_SET_ALPHA_REF                                    = 0x0000030c,
        NV4097_SET_BLEND_ENABLE                                 = 0x00000310,   // I
        NV4097_SET_BLEND_FUNC_SFACTOR                           = 0x00000314,   // I
        NV4097_SET_BLEND_FUNC_DFACTOR                           = 0x00000318,   // I
        NV4097_SET_BLEND_COLOR                                  = 0x0000031c,   // I
        NV4097_SET_BLEND_EQUATION                               = 0x00000320,   // I
        NV4097_SET_COLOR_MASK                                   = 0x00000324,
        NV4097_SET_STENCIL_TEST_ENABLE                          = 0x00000328,
        NV4097_SET_STENCIL_MASK                                 = 0x0000032c,
        NV4097_SET_STENCIL_FUNC                                 = 0x00000330,
        NV4097_SET_STENCIL_FUNC_REF                             = 0x00000334,
        NV4097_SET_STENCIL_FUNC_MASK                            = 0x00000338,
        NV4097_SET_STENCIL_OP_FAIL                              = 0x0000033c,
        NV4097_SET_STENCIL_OP_ZFAIL                             = 0x00000340,
        NV4097_SET_STENCIL_OP_ZPASS                             = 0x00000344,
        NV4097_SET_TWO_SIDED_STENCIL_TEST_ENABLE                = 0x00000348,
        NV4097_SET_BACK_STENCIL_MASK                            = 0x0000034c,
        NV4097_SET_BACK_STENCIL_FUNC                            = 0x00000350,
        NV4097_SET_BACK_STENCIL_FUNC_REF                        = 0x00000354,
        NV4097_SET_BACK_STENCIL_FUNC_MASK                       = 0x00000358,
        NV4097_SET_BACK_STENCIL_OP_FAIL                         = 0x0000035c,
        NV4097_SET_BACK_STENCIL_OP_ZFAIL                        = 0x00000360,
        NV4097_SET_BACK_STENCIL_OP_ZPASS                        = 0x00000364,
        NV4097_SET_SHADE_MODE                                   = 0x00000368,
        NV4097_SET_BLEND_ENABLE_MRT                             = 0x0000036c,
        NV4097_SET_COLOR_MASK_MRT                               = 0x00000370,
        NV4097_SET_LOGIC_OP_ENABLE                              = 0x00000374,
        NV4097_SET_LOGIC_OP                                     = 0x00000378,
        NV4097_SET_BLEND_COLOR2                                 = 0x0000037c,
        NV4097_SET_DEPTH_BOUNDS_TEST_ENABLE                     = 0x00000380,
        NV4097_SET_DEPTH_BOUNDS_MIN                             = 0x00000384,
        NV4097_SET_DEPTH_BOUNDS_MAX                             = 0x00000388,
        NV4097_SET_CLIP_MIN                                     = 0x00000394,
        NV4097_SET_CLIP_MAX                                     = 0x00000398,
        NV4097_SET_CONTROL0                                     = 0x000003b0,
        NV4097_SET_LINE_WIDTH                                   = 0x000003b8,
        NV4097_SET_LINE_SMOOTH_ENABLE                           = 0x000003bc,
        NV4097_SET_ANISO_SPREAD                                 = 0x000003c0,
        NV4097_SET_SCISSOR_HORIZONTAL                           = 0x000008c0,   // I
        NV4097_SET_SCISSOR_VERTICAL                             = 0x000008c4,   // I
        NV4097_SET_FOG_MODE                                     = 0x000008cc,
        NV4097_SET_FOG_PARAMS                                   = 0x000008d0,
        NV4097_SET_SHADER_PROGRAM                               = 0x000008e4,   // I
        NV4097_SET_VERTEX_TEXTURE_OFFSET                        = 0x00000900,
        NV4097_SET_VERTEX_TEXTURE_FORMAT                        = 0x00000904,
        NV4097_SET_VERTEX_TEXTURE_ADDRESS                       = 0x00000908,
        NV4097_SET_VERTEX_TEXTURE_CONTROL0                      = 0x0000090c,
        NV4097_SET_VERTEX_TEXTURE_CONTROL3                      = 0x00000910,
        NV4097_SET_VERTEX_TEXTURE_FILTER                        = 0x00000914,
        NV4097_SET_VERTEX_TEXTURE_IMAGE_RECT                    = 0x00000918,
        NV4097_SET_VERTEX_TEXTURE_BORDER_COLOR                  = 0x0000091c,
        NV4097_SET_VIEWPORT_HORIZONTAL                          = 0x00000a00,
        NV4097_SET_VIEWPORT_VERTICAL                            = 0x00000a04,
        NV4097_SET_POINT_CENTER_MODE                            = 0x00000a0c,
        NV4097_ZCULL_SYNC                                       = 0x00000a1c,
        NV4097_SET_VIEWPORT_OFFSET                              = 0x00000a20,   // I
        NV4097_SET_VIEWPORT_SCALE                               = 0x00000a30,   // I
        NV4097_SET_POLY_OFFSET_POINT_ENABLE                     = 0x00000a60,
        NV4097_SET_POLY_OFFSET_LINE_ENABLE                      = 0x00000a64,
        NV4097_SET_POLY_OFFSET_FILL_ENABLE                      = 0x00000a68,
        NV4097_SET_DEPTH_FUNC                                   = 0x00000a6c,   // I
        NV4097_SET_DEPTH_MASK                                   = 0x00000a70,   // I
        NV4097_SET_DEPTH_TEST_ENABLE                            = 0x00000a74,   // I
        NV4097_SET_POLYGON_OFFSET_SCALE_FACTOR                  = 0x00000a78,
        NV4097_SET_POLYGON_OFFSET_BIAS                          = 0x00000a7c,
        NV4097_SET_VERTEX_DATA_SCALED4S_M                       = 0x00000a80,
        NV4097_SET_TEXTURE_CONTROL2                             = 0x00000b00,
        NV4097_SET_TEX_COORD_CONTROL                            = 0x00000b40,
        NV4097_SET_TRANSFORM_PROGRAM                            = 0x00000b80,   // I
        NV4097_SET_SPECULAR_ENABLE                              = 0x00001428,
        NV4097_SET_TWO_SIDE_LIGHT_EN                            = 0x0000142c,
        NV4097_CLEAR_ZCULL_SURFACE                              = 0x00001438,
        NV4097_SET_PERFORMANCE_PARAMS                           = 0x00001450,
        NV4097_SET_FLAT_SHADE_OP                                = 0x00001454,
        NV4097_SET_EDGE_FLAG                                    = 0x0000145c,
        NV4097_SET_USER_CLIP_PLANE_CONTROL                      = 0x00001478,
        NV4097_SET_POLYGON_STIPPLE                              = 0x0000147c,
        NV4097_SET_POLYGON_STIPPLE_PATTERN                      = 0x00001480,
        NV4097_SET_VERTEX_DATA3F_M                              = 0x00001500,
        NV4097_SET_VERTEX_DATA_ARRAY_OFFSET                     = 0x00001680,   // I
        NV4097_INVALIDATE_VERTEX_CACHE_FILE                     = 0x00001710,
        NV4097_INVALIDATE_VERTEX_FILE                           = 0x00001714,
        NV4097_PIPE_NOP                                         = 0x00001718,
        NV4097_SET_VERTEX_DATA_BASE_OFFSET                      = 0x00001738,
        NV4097_SET_VERTEX_DATA_BASE_INDEX                       = 0x0000173c,
        NV4097_SET_VERTEX_DATA_ARRAY_FORMAT                     = 0x00001740,   // I
        NV4097_CLEAR_REPORT_VALUE                               = 0x000017c8,
        NV4097_SET_ZPASS_PIXEL_COUNT_ENABLE                     = 0x000017cc,
        NV4097_GET_REPORT                                       = 0x00001800,
        NV4097_SET_ZCULL_STATS_ENABLE                           = 0x00001804,
        NV4097_SET_BEGIN_END                                    = 0x00001808,   // I
        NV4097_ARRAY_ELEMENT16                                  = 0x0000180c,
        NV4097_ARRAY_ELEMENT32                                  = 0x00001810,
        NV4097_DRAW_ARRAYS                                      = 0x00001814,   // I
        NV4097_INLINE_ARRAY                                     = 0x00001818,   // I
        NV4097_SET_INDEX_ARRAY_ADDRESS                          = 0x0000181c,   // I
        NV4097_SET_INDEX_ARRAY_DMA                              = 0x00001820,   // I
        NV4097_DRAW_INDEX_ARRAY                                 = 0x00001824,   // I
        NV4097_SET_FRONT_POLYGON_MODE                           = 0x00001828,
        NV4097_SET_BACK_POLYGON_MODE                            = 0x0000182c,
        NV4097_SET_CULL_FACE                                    = 0x00001830,
        NV4097_SET_FRONT_FACE                                   = 0x00001834,
        NV4097_SET_POLY_SMOOTH_ENABLE                           = 0x00001838,
        NV4097_SET_CULL_FACE_ENABLE                             = 0x0000183c,   // I
        NV4097_SET_TEXTURE_CONTROL3                             = 0x00001840,   // I
        NV4097_SET_VERTEX_DATA2F_M                              = 0x00001880,   // I
        NV4097_SET_VERTEX_DATA2S_M                              = 0x00001900,
        NV4097_SET_VERTEX_DATA4UB_M                             = 0x00001940,
        NV4097_SET_VERTEX_DATA4S_M                              = 0x00001980,
        NV4097_SET_TEXTURE_OFFSET                               = 0x00001a00,   // I
        NV4097_SET_TEXTURE_FORMAT                               = 0x00001a04,   // I
        NV4097_SET_TEXTURE_ADDRESS                              = 0x00001a08,   // I
        NV4097_SET_TEXTURE_CONTROL0                             = 0x00001a0c,   // I
        NV4097_SET_TEXTURE_CONTROL1                             = 0x00001a10,   // I
        NV4097_SET_TEXTURE_FILTER                               = 0x00001a14,   // I
        NV4097_SET_TEXTURE_IMAGE_RECT                           = 0x00001a18,   // I
        NV4097_SET_TEXTURE_BORDER_COLOR                         = 0x00001a1c,
        NV4097_SET_VERTEX_DATA4F_M                              = 0x00001c00,   // I
        NV4097_SET_COLOR_KEY_COLOR                              = 0x00001d00,
        NV4097_SET_SHADER_CONTROL                               = 0x00001d60,   // I
        NV4097_SET_INDEXED_CONSTANT_READ_LIMITS                 = 0x00001d64,
        NV4097_SET_SEMAPHORE_OFFSET                             = 0x00001d6c,   // I
        NV4097_BACK_END_WRITE_SEMAPHORE_RELEASE                 = 0x00001d70,   // I
        NV4097_TEXTURE_READ_SEMAPHORE_RELEASE                   = 0x00001d74,   // I
        NV4097_SET_ZMIN_MAX_CONTROL                             = 0x00001d78,
        NV4097_SET_ANTI_ALIASING_CONTROL                        = 0x00001d7c,
        NV4097_SET_SURFACE_COMPRESSION                          = 0x00001d80,
        NV4097_SET_ZCULL_EN                                     = 0x00001d84,
        NV4097_SET_SHADER_WINDOW                                = 0x00001d88,
        NV4097_SET_ZSTENCIL_CLEAR_VALUE                         = 0x00001d8c,
        NV4097_SET_COLOR_CLEAR_VALUE                            = 0x00001d90,   // I
        NV4097_CLEAR_SURFACE                                    = 0x00001d94,   // I
        NV4097_SET_CLEAR_RECT_HORIZONTAL                        = 0x00001d98,
        NV4097_SET_CLEAR_RECT_VERTICAL                          = 0x00001d9c,
        NV4097_SET_CLIP_ID_TEST_ENABLE                          = 0x00001da4,
        NV4097_SET_RESTART_INDEX_ENABLE                         = 0x00001dac,
        NV4097_SET_RESTART_INDEX                                = 0x00001db0,
        NV4097_SET_LINE_STIPPLE                                 = 0x00001db4,
        NV4097_SET_LINE_STIPPLE_PATTERN                         = 0x00001db8,
        NV4097_SET_VERTEX_DATA1F_M                              = 0x00001e40,
        NV4097_SET_TRANSFORM_EXECUTION_MODE                     = 0x00001e94,
        NV4097_SET_RENDER_ENABLE                                = 0x00001e98,
        NV4097_SET_TRANSFORM_PROGRAM_LOAD                       = 0x00001e9c,   // I
        NV4097_SET_TRANSFORM_PROGRAM_START                      = 0x00001ea0,   // I
        NV4097_SET_ZCULL_CONTROL0                               = 0x00001ea4,
        NV4097_SET_ZCULL_CONTROL1                               = 0x00001ea8,
        NV4097_SET_SCULL_CONTROL                                = 0x00001eac,
        NV4097_SET_POINT_SIZE                                   = 0x00001ee0,
        NV4097_SET_POINT_PARAMS_ENABLE                          = 0x00001ee4,
        NV4097_SET_POINT_SPRITE_CONTROL                         = 0x00001ee8,
        NV4097_SET_TRANSFORM_TIMEOUT                            = 0x00001ef8,
        NV4097_SET_TRANSFORM_CONSTANT_LOAD                      = 0x00001efc,   // I
        NV4097_SET_TRANSFORM_CONSTANT                           = 0x00001f00,
        NV4097_SET_FREQUENCY_DIVIDER_OPERATION                  = 0x00001fc0,
        NV4097_SET_ATTRIB_COLOR                                 = 0x00001fc4,
        NV4097_SET_ATTRIB_TEX_COORD                             = 0x00001fc8,
        NV4097_SET_ATTRIB_TEX_COORD_EX                          = 0x00001fcc,
        NV4097_SET_ATTRIB_UCLIP0                                = 0x00001fd0,
        NV4097_SET_ATTRIB_UCLIP1                                = 0x00001fd4,
        NV4097_INVALIDATE_L2                                    = 0x00001fd8,
        NV4097_SET_REDUCE_DST_COLOR                             = 0x00001fe0,
        NV4097_SET_NO_PARANOID_TEXTURE_FETCHES                  = 0x00001fe8,
        NV4097_SET_SHADER_PACKER                                = 0x00001fec,
        NV4097_SET_VERTEX_ATTRIB_INPUT_MASK                     = 0x00001ff0,
        NV4097_SET_VERTEX_ATTRIB_OUTPUT_MASK                    = 0x00001ff4,   // I
        NV4097_SET_TRANSFORM_BRANCH_BITS                        = 0x00001ff8,

        // NV0039
        NV0039_SET_OBJECT                                       = 0x00002000,
        NV0039_SET_CONTEXT_DMA_NOTIFIES                         = 0x00002180,
        NV0039_SET_CONTEXT_DMA_BUFFER_IN                        = 0x00002184,
        NV0039_SET_CONTEXT_DMA_BUFFER_OUT                       = 0x00002188,
        NV0039_OFFSET_IN                                        = 0x0000230C,
        NV0039_OFFSET_OUT                                       = 0x00002310,
        NV0039_PITCH_IN                                         = 0x00002314,
        NV0039_PITCH_OUT                                        = 0x00002318,
        NV0039_LINE_LENGTH_IN                                   = 0x0000231C,
        NV0039_LINE_COUNT                                       = 0x00002320,
        NV0039_FORMAT                                           = 0x00002324,
        NV0039_BUFFER_NOTIFY                                    = 0x00002328,

        // NV3062
        NV3062_SET_OBJECT                                       = 0x00006000,
        NV3062_SET_CONTEXT_DMA_NOTIFIES                         = 0x00006180,
        NV3062_SET_CONTEXT_DMA_IMAGE_SOURCE                     = 0x00006184,
        NV3062_SET_CONTEXT_DMA_IMAGE_DESTIN                     = 0x00006188,
        NV3062_SET_COLOR_FORMAT                                 = 0x00006300,
        NV3062_SET_PITCH                                        = 0x00006304,
        NV3062_SET_OFFSET_SOURCE                                = 0x00006308,
        NV3062_SET_OFFSET_DESTIN                                = 0x0000630C,   // I

        // NV309E
        NV309E_SET_OBJECT                                       = 0x00008000,
        NV309E_SET_CONTEXT_DMA_NOTIFIES                         = 0x00008180,
        NV309E_SET_CONTEXT_DMA_IMAGE                            = 0x00008184,
        NV309E_SET_FORMAT                                       = 0x00008300,
        NV309E_SET_OFFSET                                       = 0x00008304,

        // NV308A
        NV308A_SET_OBJECT                                       = 0x0000A000,
        NV308A_SET_CONTEXT_DMA_NOTIFIES                         = 0x0000A180,
        NV308A_SET_CONTEXT_COLOR_KEY                            = 0x0000A184,
        NV308A_SET_CONTEXT_CLIP_RECTANGLE                       = 0x0000A188,
        NV308A_SET_CONTEXT_PATTERN                              = 0x0000A18C,
        NV308A_SET_CONTEXT_ROP                                  = 0x0000A190,
        NV308A_SET_CONTEXT_BETA1                                = 0x0000A194,
        NV308A_SET_CONTEXT_BETA4                                = 0x0000A198,
        NV308A_SET_CONTEXT_SURFACE                              = 0x0000A19C,
        NV308A_SET_COLOR_CONVERSION                             = 0x0000A2F8,
        NV308A_SET_OPERATION                                    = 0x0000A2FC,
        NV308A_SET_COLOR_FORMAT                                 = 0x0000A300,
        NV308A_POINT                                            = 0x0000A304,   // I
        NV308A_SIZE_OUT                                         = 0x0000A308,
        NV308A_SIZE_IN                                          = 0x0000A30C,
        NV308A_COLOR                                            = 0x0000A400,   // I

        // NV3089
        NV3089_SET_OBJECT                                       = 0x0000C000,
        NV3089_SET_CONTEXT_DMA_NOTIFIES                         = 0x0000C180,
        NV3089_SET_CONTEXT_DMA_IMAGE                            = 0x0000C184,
        NV3089_SET_CONTEXT_PATTERN                              = 0x0000C188,
        NV3089_SET_CONTEXT_ROP                                  = 0x0000C18C,
        NV3089_SET_CONTEXT_BETA1                                = 0x0000C190,
        NV3089_SET_CONTEXT_BETA4                                = 0x0000C194,
        NV3089_SET_CONTEXT_SURFACE                              = 0x0000C198,
        NV3089_SET_COLOR_CONVERSION                             = 0x0000C2FC,
        NV3089_SET_COLOR_FORMAT                                 = 0x0000C300,
        NV3089_SET_OPERATION                                    = 0x0000C304,
        NV3089_CLIP_POINT                                       = 0x0000C308,
        NV3089_CLIP_SIZE                                        = 0x0000C30C,
        NV3089_IMAGE_OUT_POINT                                  = 0x0000C310,
        NV3089_IMAGE_OUT_SIZE                                   = 0x0000C314,
        NV3089_DS_DX                                            = 0x0000C318,
        NV3089_DT_DY                                            = 0x0000C31C,
        NV3089_IMAGE_IN_SIZE                                    = 0x0000C400,
        NV3089_IMAGE_IN_FORMAT                                  = 0x0000C404,
        NV3089_IMAGE_IN_OFFSET                                  = 0x0000C408,
        NV3089_IMAGE_IN                                         = 0x0000C40C,

        // GCM
        GCM_USER_COMMAND                                        = 0x0000EB00,
        GCM_FLIP_COMMAND                                        = 0x0000FEAC,
    };

    std::unordered_map<u32, std::string> command_names {
        { NV406E_SET_REFERENCE, "NV406E_SET_REFERENCE" },
        { NV406E_SET_CONTEXT_DMA_SEMAPHORE, "NV406E_SET_CONTEXT_DMA_SEMAPHORE" },
        { NV406E_SEMAPHORE_OFFSET, "NV406E_SEMAPHORE_OFFSET" },
        { NV406E_SEMAPHORE_ACQUIRE, "NV406E_SEMAPHORE_ACQUIRE" },
        { NV406E_SEMAPHORE_RELEASE, "NV406E_SEMAPHORE_RELEASE" },
        { NV4097_SET_OBJECT, "NV4097_SET_OBJECT" },
        { NV4097_NO_OPERATION, "NV4097_NO_OPERATION" },
        { NV4097_NOTIFY, "NV4097_NOTIFY" },
        { NV4097_WAIT_FOR_IDLE, "NV4097_WAIT_FOR_IDLE" },
        { NV4097_PM_TRIGGER, "NV4097_PM_TRIGGER" },
        { NV4097_SET_CONTEXT_DMA_NOTIFIES, "NV4097_SET_CONTEXT_DMA_NOTIFIES" },
        { NV4097_SET_CONTEXT_DMA_A, "NV4097_SET_CONTEXT_DMA_A" },
        { NV4097_SET_CONTEXT_DMA_B, "NV4097_SET_CONTEXT_DMA_B" },
        { NV4097_SET_CONTEXT_DMA_COLOR_B, "NV4097_SET_CONTEXT_DMA_COLOR_B" },
        { NV4097_SET_CONTEXT_DMA_STATE, "NV4097_SET_CONTEXT_DMA_STATE" },
        { NV4097_SET_CONTEXT_DMA_COLOR_A, "NV4097_SET_CONTEXT_DMA_COLOR_A" },
        { NV4097_SET_CONTEXT_DMA_ZETA, "NV4097_SET_CONTEXT_DMA_ZETA" },
        { NV4097_SET_CONTEXT_DMA_VERTEX_A, "NV4097_SET_CONTEXT_DMA_VERTEX_A" },
        { NV4097_SET_CONTEXT_DMA_VERTEX_B, "NV4097_SET_CONTEXT_DMA_VERTEX_B" },
        { NV4097_SET_CONTEXT_DMA_SEMAPHORE, "NV4097_SET_CONTEXT_DMA_SEMAPHORE" },
        { NV4097_SET_CONTEXT_DMA_REPORT, "NV4097_SET_CONTEXT_DMA_REPORT" },
        { NV4097_SET_CONTEXT_DMA_CLIP_ID, "NV4097_SET_CONTEXT_DMA_CLIP_ID" },
        { NV4097_SET_CONTEXT_DMA_CULL_DATA, "NV4097_SET_CONTEXT_DMA_CULL_DATA" },
        { NV4097_SET_CONTEXT_DMA_COLOR_C, "NV4097_SET_CONTEXT_DMA_COLOR_C" },
        { NV4097_SET_CONTEXT_DMA_COLOR_D, "NV4097_SET_CONTEXT_DMA_COLOR_D" },
        { NV4097_SET_SURFACE_CLIP_HORIZONTAL, "NV4097_SET_SURFACE_CLIP_HORIZONTAL" },
        { NV4097_SET_SURFACE_CLIP_VERTICAL, "NV4097_SET_SURFACE_CLIP_VERTICAL" },
        { NV4097_SET_SURFACE_FORMAT, "NV4097_SET_SURFACE_FORMAT" },
        { NV4097_SET_SURFACE_PITCH_A, "NV4097_SET_SURFACE_PITCH_A" },
        { NV4097_SET_SURFACE_COLOR_AOFFSET, "NV4097_SET_SURFACE_COLOR_AOFFSET" },
        { NV4097_SET_SURFACE_ZETA_OFFSET, "NV4097_SET_SURFACE_ZETA_OFFSET" },
        { NV4097_SET_SURFACE_COLOR_BOFFSET, "NV4097_SET_SURFACE_COLOR_BOFFSET" },
        { NV4097_SET_SURFACE_PITCH_B, "NV4097_SET_SURFACE_PITCH_B" },
        { NV4097_SET_SURFACE_COLOR_TARGET, "NV4097_SET_SURFACE_COLOR_TARGET" },
        { NV4097_SET_SURFACE_PITCH_Z, "NV4097_SET_SURFACE_PITCH_Z" },
        { NV4097_INVALIDATE_ZCULL, "NV4097_INVALIDATE_ZCULL" },
        { NV4097_SET_CYLINDRICAL_WRAP, "NV4097_SET_CYLINDRICAL_WRAP" },
        { NV4097_SET_CYLINDRICAL_WRAP1, "NV4097_SET_CYLINDRICAL_WRAP1" },
        { NV4097_SET_SURFACE_PITCH_C, "NV4097_SET_SURFACE_PITCH_C" },
        { NV4097_SET_SURFACE_PITCH_D, "NV4097_SET_SURFACE_PITCH_D" },
        { NV4097_SET_SURFACE_COLOR_COFFSET, "NV4097_SET_SURFACE_COLOR_COFFSET" },
        { NV4097_SET_SURFACE_COLOR_DOFFSET, "NV4097_SET_SURFACE_COLOR_DOFFSET" },
        { NV4097_SET_WINDOW_OFFSET, "NV4097_SET_WINDOW_OFFSET" },
        { NV4097_SET_WINDOW_CLIP_TYPE, "NV4097_SET_WINDOW_CLIP_TYPE" },
        { NV4097_SET_WINDOW_CLIP_HORIZONTAL, "NV4097_SET_WINDOW_CLIP_HORIZONTAL" },
        { NV4097_SET_WINDOW_CLIP_VERTICAL, "NV4097_SET_WINDOW_CLIP_VERTICAL" },
        { NV4097_SET_DITHER_ENABLE, "NV4097_SET_DITHER_ENABLE" },
        { NV4097_SET_ALPHA_TEST_ENABLE, "NV4097_SET_ALPHA_TEST_ENABLE" },
        { NV4097_SET_ALPHA_FUNC, "NV4097_SET_ALPHA_FUNC" },
        { NV4097_SET_ALPHA_REF, "NV4097_SET_ALPHA_REF" },
        { NV4097_SET_BLEND_ENABLE, "NV4097_SET_BLEND_ENABLE" },
        { NV4097_SET_BLEND_FUNC_SFACTOR, "NV4097_SET_BLEND_FUNC_SFACTOR" },
        { NV4097_SET_BLEND_FUNC_DFACTOR, "NV4097_SET_BLEND_FUNC_DFACTOR" },
        { NV4097_SET_BLEND_COLOR, "NV4097_SET_BLEND_COLOR" },
        { NV4097_SET_BLEND_EQUATION, "NV4097_SET_BLEND_EQUATION" },
        { NV4097_SET_COLOR_MASK, "NV4097_SET_COLOR_MASK" },
        { NV4097_SET_STENCIL_TEST_ENABLE, "NV4097_SET_STENCIL_TEST_ENABLE" },
        { NV4097_SET_STENCIL_MASK, "NV4097_SET_STENCIL_MASK" },
        { NV4097_SET_STENCIL_FUNC, "NV4097_SET_STENCIL_FUNC" },
        { NV4097_SET_STENCIL_FUNC_REF, "NV4097_SET_STENCIL_FUNC_REF" },
        { NV4097_SET_STENCIL_FUNC_MASK, "NV4097_SET_STENCIL_FUNC_MASK" },
        { NV4097_SET_STENCIL_OP_FAIL, "NV4097_SET_STENCIL_OP_FAIL" },
        { NV4097_SET_STENCIL_OP_ZFAIL, "NV4097_SET_STENCIL_OP_ZFAIL" },
        { NV4097_SET_STENCIL_OP_ZPASS, "NV4097_SET_STENCIL_OP_ZPASS" },
        { NV4097_SET_TWO_SIDED_STENCIL_TEST_ENABLE, "NV4097_SET_TWO_SIDED_STENCIL_TEST_ENABLE" },
        { NV4097_SET_BACK_STENCIL_MASK, "NV4097_SET_BACK_STENCIL_MASK" },
        { NV4097_SET_BACK_STENCIL_FUNC, "NV4097_SET_BACK_STENCIL_FUNC" },
        { NV4097_SET_BACK_STENCIL_FUNC_REF, "NV4097_SET_BACK_STENCIL_FUNC_REF" },
        { NV4097_SET_BACK_STENCIL_FUNC_MASK, "NV4097_SET_BACK_STENCIL_FUNC_MASK" },
        { NV4097_SET_BACK_STENCIL_OP_FAIL, "NV4097_SET_BACK_STENCIL_OP_FAIL" },
        { NV4097_SET_BACK_STENCIL_OP_ZFAIL, "NV4097_SET_BACK_STENCIL_OP_ZFAIL" },
        { NV4097_SET_BACK_STENCIL_OP_ZPASS, "NV4097_SET_BACK_STENCIL_OP_ZPASS" },
        { NV4097_SET_SHADE_MODE, "NV4097_SET_SHADE_MODE" },
        { NV4097_SET_BLEND_ENABLE_MRT, "NV4097_SET_BLEND_ENABLE_MRT" },
        { NV4097_SET_COLOR_MASK_MRT, "NV4097_SET_COLOR_MASK_MRT" },
        { NV4097_SET_LOGIC_OP_ENABLE, "NV4097_SET_LOGIC_OP_ENABLE" },
        { NV4097_SET_LOGIC_OP, "NV4097_SET_LOGIC_OP" },
        { NV4097_SET_BLEND_COLOR2, "NV4097_SET_BLEND_COLOR2" },
        { NV4097_SET_DEPTH_BOUNDS_TEST_ENABLE, "NV4097_SET_DEPTH_BOUNDS_TEST_ENABLE" },
        { NV4097_SET_DEPTH_BOUNDS_MIN, "NV4097_SET_DEPTH_BOUNDS_MIN" },
        { NV4097_SET_DEPTH_BOUNDS_MAX, "NV4097_SET_DEPTH_BOUNDS_MAX" },
        { NV4097_SET_CLIP_MIN, "NV4097_SET_CLIP_MIN" },
        { NV4097_SET_CLIP_MAX, "NV4097_SET_CLIP_MAX" },
        { NV4097_SET_CONTROL0, "NV4097_SET_CONTROL0" },
        { NV4097_SET_LINE_WIDTH, "NV4097_SET_LINE_WIDTH" },
        { NV4097_SET_LINE_SMOOTH_ENABLE, "NV4097_SET_LINE_SMOOTH_ENABLE" },
        { NV4097_SET_ANISO_SPREAD, "NV4097_SET_ANISO_SPREAD" },
        { NV4097_SET_SCISSOR_HORIZONTAL, "NV4097_SET_SCISSOR_HORIZONTAL" },
        { NV4097_SET_SCISSOR_VERTICAL, "NV4097_SET_SCISSOR_VERTICAL" },
        { NV4097_SET_FOG_MODE, "NV4097_SET_FOG_MODE" },
        { NV4097_SET_FOG_PARAMS, "NV4097_SET_FOG_PARAMS" },
        { NV4097_SET_SHADER_PROGRAM, "NV4097_SET_SHADER_PROGRAM" },
        { NV4097_SET_VERTEX_TEXTURE_OFFSET, "NV4097_SET_VERTEX_TEXTURE_OFFSET" },
        { NV4097_SET_VERTEX_TEXTURE_FORMAT, "NV4097_SET_VERTEX_TEXTURE_FORMAT" },
        { NV4097_SET_VERTEX_TEXTURE_ADDRESS, "NV4097_SET_VERTEX_TEXTURE_ADDRESS" },
        { NV4097_SET_VERTEX_TEXTURE_CONTROL0, "NV4097_SET_VERTEX_TEXTURE_CONTROL0" },
        { NV4097_SET_VERTEX_TEXTURE_CONTROL3, "NV4097_SET_VERTEX_TEXTURE_CONTROL3" },
        { NV4097_SET_VERTEX_TEXTURE_FILTER, "NV4097_SET_VERTEX_TEXTURE_FILTER" },
        { NV4097_SET_VERTEX_TEXTURE_IMAGE_RECT, "NV4097_SET_VERTEX_TEXTURE_IMAGE_RECT" },
        { NV4097_SET_VERTEX_TEXTURE_BORDER_COLOR, "NV4097_SET_VERTEX_TEXTURE_BORDER_COLOR" },
        { NV4097_SET_VIEWPORT_HORIZONTAL, "NV4097_SET_VIEWPORT_HORIZONTAL" },
        { NV4097_SET_VIEWPORT_VERTICAL, "NV4097_SET_VIEWPORT_VERTICAL" },
        { NV4097_SET_POINT_CENTER_MODE, "NV4097_SET_POINT_CENTER_MODE" },
        { NV4097_ZCULL_SYNC, "NV4097_ZCULL_SYNC" },
        { NV4097_SET_VIEWPORT_OFFSET, "NV4097_SET_VIEWPORT_OFFSET" },
        { NV4097_SET_VIEWPORT_SCALE, "NV4097_SET_VIEWPORT_SCALE" },
        { NV4097_SET_POLY_OFFSET_POINT_ENABLE, "NV4097_SET_POLY_OFFSET_POINT_ENABLE" },
        { NV4097_SET_POLY_OFFSET_LINE_ENABLE, "NV4097_SET_POLY_OFFSET_LINE_ENABLE" },
        { NV4097_SET_POLY_OFFSET_FILL_ENABLE, "NV4097_SET_POLY_OFFSET_FILL_ENABLE" },
        { NV4097_SET_DEPTH_FUNC, "NV4097_SET_DEPTH_FUNC" },
        { NV4097_SET_DEPTH_MASK, "NV4097_SET_DEPTH_MASK" },
        { NV4097_SET_DEPTH_TEST_ENABLE, "NV4097_SET_DEPTH_TEST_ENABLE" },
        { NV4097_SET_POLYGON_OFFSET_SCALE_FACTOR, "NV4097_SET_POLYGON_OFFSET_SCALE_FACTOR" },
        { NV4097_SET_POLYGON_OFFSET_BIAS, "NV4097_SET_POLYGON_OFFSET_BIAS" },
        { NV4097_SET_VERTEX_DATA_SCALED4S_M, "NV4097_SET_VERTEX_DATA_SCALED4S_M" },
        { NV4097_SET_TEXTURE_CONTROL2, "NV4097_SET_TEXTURE_CONTROL2" },
        { NV4097_SET_TEX_COORD_CONTROL, "NV4097_SET_TEX_COORD_CONTROL" },
        { NV4097_SET_TRANSFORM_PROGRAM, "NV4097_SET_TRANSFORM_PROGRAM" },
        { NV4097_SET_SPECULAR_ENABLE, "NV4097_SET_SPECULAR_ENABLE" },
        { NV4097_SET_TWO_SIDE_LIGHT_EN, "NV4097_SET_TWO_SIDE_LIGHT_EN" },
        { NV4097_CLEAR_ZCULL_SURFACE, "NV4097_CLEAR_ZCULL_SURFACE" },
        { NV4097_SET_PERFORMANCE_PARAMS, "NV4097_SET_PERFORMANCE_PARAMS" },
        { NV4097_SET_FLAT_SHADE_OP, "NV4097_SET_FLAT_SHADE_OP" },
        { NV4097_SET_EDGE_FLAG, "NV4097_SET_EDGE_FLAG" },
        { NV4097_SET_USER_CLIP_PLANE_CONTROL, "NV4097_SET_USER_CLIP_PLANE_CONTROL" },
        { NV4097_SET_POLYGON_STIPPLE, "NV4097_SET_POLYGON_STIPPLE" },
        { NV4097_SET_POLYGON_STIPPLE_PATTERN, "NV4097_SET_POLYGON_STIPPLE_PATTERN" },
        { NV4097_SET_VERTEX_DATA3F_M, "NV4097_SET_VERTEX_DATA3F_M" },
        { NV4097_SET_VERTEX_DATA_ARRAY_OFFSET, "NV4097_SET_VERTEX_DATA_ARRAY_OFFSET" },
        { NV4097_INVALIDATE_VERTEX_CACHE_FILE, "NV4097_INVALIDATE_VERTEX_CACHE_FILE" },
        { NV4097_INVALIDATE_VERTEX_FILE, "NV4097_INVALIDATE_VERTEX_FILE" },
        { NV4097_PIPE_NOP, "NV4097_PIPE_NOP" },
        { NV4097_SET_VERTEX_DATA_BASE_OFFSET, "NV4097_SET_VERTEX_DATA_BASE_OFFSET" },
        { NV4097_SET_VERTEX_DATA_BASE_INDEX, "NV4097_SET_VERTEX_DATA_BASE_INDEX" },
        { NV4097_SET_VERTEX_DATA_ARRAY_FORMAT, "NV4097_SET_VERTEX_DATA_ARRAY_FORMAT" },
        { NV4097_CLEAR_REPORT_VALUE, "NV4097_CLEAR_REPORT_VALUE" },
        { NV4097_SET_ZPASS_PIXEL_COUNT_ENABLE, "NV4097_SET_ZPASS_PIXEL_COUNT_ENABLE" },
        { NV4097_GET_REPORT, "NV4097_GET_REPORT" },
        { NV4097_SET_ZCULL_STATS_ENABLE, "NV4097_SET_ZCULL_STATS_ENABLE" },
        { NV4097_SET_BEGIN_END, "NV4097_SET_BEGIN_END" },
        { NV4097_ARRAY_ELEMENT16, "NV4097_ARRAY_ELEMENT16" },
        { NV4097_ARRAY_ELEMENT32, "NV4097_ARRAY_ELEMENT32" },
        { NV4097_DRAW_ARRAYS, "NV4097_DRAW_ARRAYS" },
        { NV4097_INLINE_ARRAY, "NV4097_INLINE_ARRAY" },
        { NV4097_SET_INDEX_ARRAY_ADDRESS, "NV4097_SET_INDEX_ARRAY_ADDRESS" },
        { NV4097_SET_INDEX_ARRAY_DMA, "NV4097_SET_INDEX_ARRAY_DMA" },
        { NV4097_DRAW_INDEX_ARRAY, "NV4097_DRAW_INDEX_ARRAY" },
        { NV4097_SET_FRONT_POLYGON_MODE, "NV4097_SET_FRONT_POLYGON_MODE" },
        { NV4097_SET_BACK_POLYGON_MODE, "NV4097_SET_BACK_POLYGON_MODE" },
        { NV4097_SET_CULL_FACE, "NV4097_SET_CULL_FACE" },
        { NV4097_SET_FRONT_FACE, "NV4097_SET_FRONT_FACE" },
        { NV4097_SET_POLY_SMOOTH_ENABLE, "NV4097_SET_POLY_SMOOTH_ENABLE" },
        { NV4097_SET_CULL_FACE_ENABLE, "NV4097_SET_CULL_FACE_ENABLE" },
        { NV4097_SET_TEXTURE_CONTROL3, "NV4097_SET_TEXTURE_CONTROL3" },
        { NV4097_SET_VERTEX_DATA2F_M, "NV4097_SET_VERTEX_DATA2F_M" },
        { NV4097_SET_VERTEX_DATA2S_M, "NV4097_SET_VERTEX_DATA2S_M" },
        { NV4097_SET_VERTEX_DATA4UB_M, "NV4097_SET_VERTEX_DATA4UB_M" },
        { NV4097_SET_VERTEX_DATA4S_M, "NV4097_SET_VERTEX_DATA4S_M" },
        { NV4097_SET_TEXTURE_OFFSET, "NV4097_SET_TEXTURE_OFFSET" },
        { NV4097_SET_TEXTURE_FORMAT, "NV4097_SET_TEXTURE_FORMAT" },
        { NV4097_SET_TEXTURE_ADDRESS, "NV4097_SET_TEXTURE_ADDRESS" },
        { NV4097_SET_TEXTURE_CONTROL0, "NV4097_SET_TEXTURE_CONTROL0" },
        { NV4097_SET_TEXTURE_CONTROL1, "NV4097_SET_TEXTURE_CONTROL1" },
        { NV4097_SET_TEXTURE_FILTER, "NV4097_SET_TEXTURE_FILTER" },
        { NV4097_SET_TEXTURE_IMAGE_RECT, "NV4097_SET_TEXTURE_IMAGE_RECT" },
        { NV4097_SET_TEXTURE_BORDER_COLOR, "NV4097_SET_TEXTURE_BORDER_COLOR" },
        { NV4097_SET_VERTEX_DATA4F_M, "NV4097_SET_VERTEX_DATA4F_M" },
        { NV4097_SET_COLOR_KEY_COLOR, "NV4097_SET_COLOR_KEY_COLOR" },
        { NV4097_SET_SHADER_CONTROL, "NV4097_SET_SHADER_CONTROL" },
        { NV4097_SET_INDEXED_CONSTANT_READ_LIMITS, "NV4097_SET_INDEXED_CONSTANT_READ_LIMITS" },
        { NV4097_SET_SEMAPHORE_OFFSET, "NV4097_SET_SEMAPHORE_OFFSET" },
        { NV4097_BACK_END_WRITE_SEMAPHORE_RELEASE, "NV4097_BACK_END_WRITE_SEMAPHORE_RELEASE" },
        { NV4097_TEXTURE_READ_SEMAPHORE_RELEASE, "NV4097_TEXTURE_READ_SEMAPHORE_RELEASE" },
        { NV4097_SET_ZMIN_MAX_CONTROL, "NV4097_SET_ZMIN_MAX_CONTROL" },
        { NV4097_SET_ANTI_ALIASING_CONTROL, "NV4097_SET_ANTI_ALIASING_CONTROL" },
        { NV4097_SET_SURFACE_COMPRESSION, "NV4097_SET_SURFACE_COMPRESSION" },
        { NV4097_SET_ZCULL_EN, "NV4097_SET_ZCULL_EN" },
        { NV4097_SET_SHADER_WINDOW, "NV4097_SET_SHADER_WINDOW" },
        { NV4097_SET_ZSTENCIL_CLEAR_VALUE, "NV4097_SET_ZSTENCIL_CLEAR_VALUE" },
        { NV4097_SET_COLOR_CLEAR_VALUE, "NV4097_SET_COLOR_CLEAR_VALUE" },
        { NV4097_CLEAR_SURFACE, "NV4097_CLEAR_SURFACE" },
        { NV4097_SET_CLEAR_RECT_HORIZONTAL, "NV4097_SET_CLEAR_RECT_HORIZONTAL" },
        { NV4097_SET_CLEAR_RECT_VERTICAL, "NV4097_SET_CLEAR_RECT_VERTICAL" },
        { NV4097_SET_CLIP_ID_TEST_ENABLE, "NV4097_SET_CLIP_ID_TEST_ENABLE" },
        { NV4097_SET_RESTART_INDEX_ENABLE, "NV4097_SET_RESTART_INDEX_ENABLE" },
        { NV4097_SET_RESTART_INDEX, "NV4097_SET_RESTART_INDEX" },
        { NV4097_SET_LINE_STIPPLE, "NV4097_SET_LINE_STIPPLE" },
        { NV4097_SET_LINE_STIPPLE_PATTERN, "NV4097_SET_LINE_STIPPLE_PATTERN" },
        { NV4097_SET_VERTEX_DATA1F_M, "NV4097_SET_VERTEX_DATA1F_M" },
        { NV4097_SET_TRANSFORM_EXECUTION_MODE, "NV4097_SET_TRANSFORM_EXECUTION_MODE" },
        { NV4097_SET_RENDER_ENABLE, "NV4097_SET_RENDER_ENABLE" },
        { NV4097_SET_TRANSFORM_PROGRAM_LOAD, "NV4097_SET_TRANSFORM_PROGRAM_LOAD" },
        { NV4097_SET_TRANSFORM_PROGRAM_START, "NV4097_SET_TRANSFORM_PROGRAM_START" },
        { NV4097_SET_ZCULL_CONTROL0, "NV4097_SET_ZCULL_CONTROL0" },
        { NV4097_SET_ZCULL_CONTROL1, "NV4097_SET_ZCULL_CONTROL1" },
        { NV4097_SET_SCULL_CONTROL, "NV4097_SET_SCULL_CONTROL" },
        { NV4097_SET_POINT_SIZE, "NV4097_SET_POINT_SIZE" },
        { NV4097_SET_POINT_PARAMS_ENABLE, "NV4097_SET_POINT_PARAMS_ENABLE" },
        { NV4097_SET_POINT_SPRITE_CONTROL, "NV4097_SET_POINT_SPRITE_CONTROL" },
        { NV4097_SET_TRANSFORM_TIMEOUT, "NV4097_SET_TRANSFORM_TIMEOUT" },
        { NV4097_SET_TRANSFORM_CONSTANT_LOAD, "NV4097_SET_TRANSFORM_CONSTANT_LOAD" },
        { NV4097_SET_TRANSFORM_CONSTANT, "NV4097_SET_TRANSFORM_CONSTANT" },
        { NV4097_SET_FREQUENCY_DIVIDER_OPERATION, "NV4097_SET_FREQUENCY_DIVIDER_OPERATION" },
        { NV4097_SET_ATTRIB_COLOR, "NV4097_SET_ATTRIB_COLOR" },
        { NV4097_SET_ATTRIB_TEX_COORD, "NV4097_SET_ATTRIB_TEX_COORD" },
        { NV4097_SET_ATTRIB_TEX_COORD_EX, "NV4097_SET_ATTRIB_TEX_COORD_EX" },
        { NV4097_SET_ATTRIB_UCLIP0, "NV4097_SET_ATTRIB_UCLIP0" },
        { NV4097_SET_ATTRIB_UCLIP1, "NV4097_SET_ATTRIB_UCLIP1" },
        { NV4097_INVALIDATE_L2, "NV4097_INVALIDATE_L2" },
        { NV4097_SET_REDUCE_DST_COLOR, "NV4097_SET_REDUCE_DST_COLOR" },
        { NV4097_SET_NO_PARANOID_TEXTURE_FETCHES, "NV4097_SET_NO_PARANOID_TEXTURE_FETCHES" },
        { NV4097_SET_SHADER_PACKER, "NV4097_SET_SHADER_PACKER" },
        { NV4097_SET_VERTEX_ATTRIB_INPUT_MASK, "NV4097_SET_VERTEX_ATTRIB_INPUT_MASK" },
        { NV4097_SET_VERTEX_ATTRIB_OUTPUT_MASK, "NV4097_SET_VERTEX_ATTRIB_OUTPUT_MASK" },
        { NV4097_SET_TRANSFORM_BRANCH_BITS, "NV4097_SET_TRANSFORM_BRANCH_BITS" },
        { NV0039_SET_OBJECT, "NV0039_SET_OBJECT" },
        { NV0039_SET_CONTEXT_DMA_NOTIFIES, "NV0039_SET_CONTEXT_DMA_NOTIFIES" },
        { NV0039_SET_CONTEXT_DMA_BUFFER_IN, "NV0039_SET_CONTEXT_DMA_BUFFER_IN" },
        { NV0039_SET_CONTEXT_DMA_BUFFER_OUT, "NV0039_SET_CONTEXT_DMA_BUFFER_OUT" },
        { NV0039_OFFSET_IN, "NV0039_OFFSET_IN" },
        { NV0039_OFFSET_OUT, "NV0039_OFFSET_OUT" },
        { NV0039_PITCH_IN, "NV0039_PITCH_IN" },
        { NV0039_PITCH_OUT, "NV0039_PITCH_OUT" },
        { NV0039_LINE_LENGTH_IN, "NV0039_LINE_LENGTH_IN" },
        { NV0039_LINE_COUNT, "NV0039_LINE_COUNT" },
        { NV0039_FORMAT, "NV0039_FORMAT" },
        { NV0039_BUFFER_NOTIFY, "NV0039_BUFFER_NOTIFY" },
        { NV3062_SET_OBJECT, "NV3062_SET_OBJECT" },
        { NV3062_SET_CONTEXT_DMA_NOTIFIES, "NV3062_SET_CONTEXT_DMA_NOTIFIES" },
        { NV3062_SET_CONTEXT_DMA_IMAGE_SOURCE, "NV3062_SET_CONTEXT_DMA_IMAGE_SOURCE" },
        { NV3062_SET_CONTEXT_DMA_IMAGE_DESTIN, "NV3062_SET_CONTEXT_DMA_IMAGE_DESTIN" },
        { NV3062_SET_COLOR_FORMAT, "NV3062_SET_COLOR_FORMAT" },
        { NV3062_SET_PITCH, "NV3062_SET_PITCH" },
        { NV3062_SET_OFFSET_SOURCE, "NV3062_SET_OFFSET_SOURCE" },
        { NV3062_SET_OFFSET_DESTIN, "NV3062_SET_OFFSET_DESTIN" },
        { NV309E_SET_OBJECT, "NV309E_SET_OBJECT" },
        { NV309E_SET_CONTEXT_DMA_NOTIFIES, "NV309E_SET_CONTEXT_DMA_NOTIFIES" },
        { NV309E_SET_CONTEXT_DMA_IMAGE, "NV309E_SET_CONTEXT_DMA_IMAGE" },
        { NV309E_SET_FORMAT, "NV309E_SET_FORMAT" },
        { NV309E_SET_OFFSET, "NV309E_SET_OFFSET" },
        { NV308A_SET_OBJECT, "NV308A_SET_OBJECT" },
        { NV308A_SET_CONTEXT_DMA_NOTIFIES, "NV308A_SET_CONTEXT_DMA_NOTIFIES" },
        { NV308A_SET_CONTEXT_COLOR_KEY, "NV308A_SET_CONTEXT_COLOR_KEY" },
        { NV308A_SET_CONTEXT_CLIP_RECTANGLE, "NV308A_SET_CONTEXT_CLIP_RECTANGLE" },
        { NV308A_SET_CONTEXT_PATTERN, "NV308A_SET_CONTEXT_PATTERN" },
        { NV308A_SET_CONTEXT_ROP, "NV308A_SET_CONTEXT_ROP" },
        { NV308A_SET_CONTEXT_BETA1, "NV308A_SET_CONTEXT_BETA1" },
        { NV308A_SET_CONTEXT_BETA4, "NV308A_SET_CONTEXT_BETA4" },
        { NV308A_SET_CONTEXT_SURFACE, "NV308A_SET_CONTEXT_SURFACE" },
        { NV308A_SET_COLOR_CONVERSION, "NV308A_SET_COLOR_CONVERSION" },
        { NV308A_SET_OPERATION, "NV308A_SET_OPERATION" },
        { NV308A_SET_COLOR_FORMAT, "NV308A_SET_COLOR_FORMAT" },
        { NV308A_POINT, "NV308A_POINT" },
        { NV308A_SIZE_OUT, "NV308A_SIZE_OUT" },
        { NV308A_SIZE_IN, "NV308A_SIZE_IN" },
        { NV308A_COLOR, "NV308A_COLOR" },
        { NV3089_SET_OBJECT, "NV3089_SET_OBJECT" },
        { NV3089_SET_CONTEXT_DMA_NOTIFIES, "NV3089_SET_CONTEXT_DMA_NOTIFIES" },
        { NV3089_SET_CONTEXT_DMA_IMAGE, "NV3089_SET_CONTEXT_DMA_IMAGE" },
        { NV3089_SET_CONTEXT_PATTERN, "NV3089_SET_CONTEXT_PATTERN" },
        { NV3089_SET_CONTEXT_ROP, "NV3089_SET_CONTEXT_ROP" },
        { NV3089_SET_CONTEXT_BETA1, "NV3089_SET_CONTEXT_BETA1" },
        { NV3089_SET_CONTEXT_BETA4, "NV3089_SET_CONTEXT_BETA4" },
        { NV3089_SET_CONTEXT_SURFACE, "NV3089_SET_CONTEXT_SURFACE" },
        { NV3089_SET_COLOR_CONVERSION, "NV3089_SET_COLOR_CONVERSION" },
        { NV3089_SET_COLOR_FORMAT, "NV3089_SET_COLOR_FORMAT" },
        { NV3089_SET_OPERATION, "NV3089_SET_OPERATION" },
        { NV3089_CLIP_POINT, "NV3089_CLIP_POINT" },
        { NV3089_CLIP_SIZE, "NV3089_CLIP_SIZE" },
        { NV3089_IMAGE_OUT_POINT, "NV3089_IMAGE_OUT_POINT" },
        { NV3089_IMAGE_OUT_SIZE, "NV3089_IMAGE_OUT_SIZE" },
        { NV3089_DS_DX, "NV3089_DS_DX" },
        { NV3089_DT_DY, "NV3089_DT_DY" },
        { NV3089_IMAGE_IN_SIZE, "NV3089_IMAGE_IN_SIZE" },
        { NV3089_IMAGE_IN_FORMAT, "NV3089_IMAGE_IN_FORMAT" },
        { NV3089_IMAGE_IN_OFFSET, "NV3089_IMAGE_IN_OFFSET" },
        { NV3089_IMAGE_IN, "NV3089_IMAGE_IN" },
    };
};
