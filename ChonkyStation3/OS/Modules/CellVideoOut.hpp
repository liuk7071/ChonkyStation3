#pragma once

#include <common.hpp>
#include <logger.hpp>
#include <BEField.hpp>

#include <unordered_map>

#include <CellTypes.hpp>


// Circular dependency
class PlayStation3;

using namespace CellTypes;

class CellVideoOut {
public:
    CellVideoOut(PlayStation3* ps3) : ps3(ps3) {}
    PlayStation3* ps3;

    enum CellVideoOutResolutionId : u32 {
        CELL_VIDEO_OUT_RESOLUTION_UNDEFINED = 0,
        CELL_VIDEO_OUT_RESOLUTION_1080 = 1,
        CELL_VIDEO_OUT_RESOLUTION_720 = 2,
        CELL_VIDEO_OUT_RESOLUTION_480 = 4,
        CELL_VIDEO_OUT_RESOLUTION_576 = 5,
        CELL_VIDEO_OUT_RESOLUTION_1600x1080 = 0xa,
        CELL_VIDEO_OUT_RESOLUTION_1440x1080 = 0xb,
        CELL_VIDEO_OUT_RESOLUTION_1280x1080 = 0xc,
        CELL_VIDEO_OUT_RESOLUTION_960x1080 = 0xd,
        CELL_VIDEO_OUT_RESOLUTION_720_3D_FRAME_PACKING = 0x81,
        CELL_VIDEO_OUT_RESOLUTION_1024x720_3D_FRAME_PACKING = 0x88,
        CELL_VIDEO_OUT_RESOLUTION_960x720_3D_FRAME_PACKING = 0x89,
        CELL_VIDEO_OUT_RESOLUTION_800x720_3D_FRAME_PACKING = 0x8a,
        CELL_VIDEO_OUT_RESOLUTION_640x720_3D_FRAME_PACKING = 0x8b,
        CELL_VIDEO_OUT_RESOLUTION_720_DUALVIEW_FRAME_PACKING = 0x91,
        CELL_VIDEO_OUT_RESOLUTION_720_SIMULVIEW_FRAME_PACKING = 0x91,
        CELL_VIDEO_OUT_RESOLUTION_1024x720_DUALVIEW_FRAME_PACKING = 0x98,
        CELL_VIDEO_OUT_RESOLUTION_1024x720_SIMULVIEW_FRAME_PACKING = 0x98,
        CELL_VIDEO_OUT_RESOLUTION_960x720_DUALVIEW_FRAME_PACKING = 0x99,
        CELL_VIDEO_OUT_RESOLUTION_960x720_SIMULVIEW_FRAME_PACKING = 0x99,
        CELL_VIDEO_OUT_RESOLUTION_800x720_DUALVIEW_FRAME_PACKING = 0x9a,
        CELL_VIDEO_OUT_RESOLUTION_800x720_SIMULVIEW_FRAME_PACKING = 0x9a,
        CELL_VIDEO_OUT_RESOLUTION_640x720_DUALVIEW_FRAME_PACKING = 0x9b,
        CELL_VIDEO_OUT_RESOLUTION_640x720_SIMULVIEW_FRAME_PACKING = 0x9b,
    };

    enum CellVideoOutOutputState : u32 {
        CELL_VIDEO_OUT_OUTPUT_STATE_ENABLED,
        CELL_VIDEO_OUT_OUTPUT_STATE_DISABLED,
        CELL_VIDEO_OUT_OUTPUT_STATE_PREPARING,
    };

    enum CellVideoOutColorSpace : u32 {
        CELL_VIDEO_OUT_COLOR_SPACE_RGB = 0x01,
        CELL_VIDEO_OUT_COLOR_SPACE_YUV = 0x02,
        CELL_VIDEO_OUT_COLOR_SPACE_XVYCC = 0x04,
    };

    enum CellVideoOutScanMode : u32 {
        CELL_VIDEO_OUT_SCAN_MODE_INTERLACE,
        CELL_VIDEO_OUT_SCAN_MODE_PROGRESSIVE,
    };

    enum CellVideoOutRefreshRate : u32 {
        CELL_VIDEO_OUT_REFRESH_RATE_AUTO = 0x0000,
        CELL_VIDEO_OUT_REFRESH_RATE_59_94HZ = 0x0001,
        CELL_VIDEO_OUT_REFRESH_RATE_50HZ = 0x0002,
        CELL_VIDEO_OUT_REFRESH_RATE_60HZ = 0x0004,
        CELL_VIDEO_OUT_REFRESH_RATE_30HZ = 0x0008,
    };

    enum CellVideoOutDisplayConversion : u32 {
        CELL_VIDEO_OUT_DISPLAY_CONVERSION_NONE = 0x00,
        CELL_VIDEO_OUT_DISPLAY_CONVERSION_TO_WXGA = 0x01,
        CELL_VIDEO_OUT_DISPLAY_CONVERSION_TO_SXGA = 0x02,
        CELL_VIDEO_OUT_DISPLAY_CONVERSION_TO_WUXGA = 0x03,
        CELL_VIDEO_OUT_DISPLAY_CONVERSION_TO_1080 = 0x05,
        CELL_VIDEO_OUT_DISPLAY_CONVERSION_TO_REMOTEPLAY = 0x10,
        CELL_VIDEO_OUT_DISPLAY_CONVERSION_TO_720_3D_FRAME_PACKING = 0x80,
    };

    enum CellVideoOutPortType : u32 {
        CELL_VIDEO_OUT_PORT_NONE = 0x00,
        CELL_VIDEO_OUT_PORT_HDMI = 0x01,
        CELL_VIDEO_OUT_PORT_NETWORK = 0x41,
        CELL_VIDEO_OUT_PORT_COMPOSITE_S = 0x81,
        CELL_VIDEO_OUT_PORT_D = 0x82,
        CELL_VIDEO_OUT_PORT_COMPONENT = 0x83,
        CELL_VIDEO_OUT_PORT_RGB = 0x84,
        CELL_VIDEO_OUT_PORT_AVMULTI_SCART = 0x85,
        CELL_VIDEO_OUT_PORT_DSUB = 0x86,
    };

    enum CellVideoOutDisplayAspect : u32 {
        CELL_VIDEO_OUT_ASPECT_AUTO,
        CELL_VIDEO_OUT_ASPECT_4_3,
        CELL_VIDEO_OUT_ASPECT_16_9,
    };

    struct CellVideoOutResolution {
        BEField<u16> width;
        BEField<u16> height;
    };

    struct CellVideoOutResolutionLE {   // No byte swapping
        u16 width;
        u16 height;
    };

    struct CellVideoOutConfiguration {
        u8 res_id;
        u8 format;
        u8 aspect;
        u8 reserved[9];
        BEField<u32> pitch;
    };
    CellVideoOutConfiguration video_config;

    struct CellVideoOutDisplayMode {
        u8 res_id;
        u8 scan_mode;
        u8 conversion;
        u8 aspect;
        u8 reserved[2];
        BEField<u16> refresh_rates;
    };

    struct CellVideoOutState {
        u8 state;
        u8 color_space;
        u8 reserved[6];
        CellVideoOutDisplayMode display_mode;
    };

    struct CellVideoOutColorInfo {
        BEField<u16> red_x;
        BEField<u16> red_y;
        BEField<u16> green_x;
        BEField<u16> green_y;
        BEField<u16> blue_x;
        BEField<u16> blue_y;
        BEField<u16> white_x;
        BEField<u16> white_y;
        BEField<u32> gamma;
    };

    struct CellVideoOutDeviceInfo {
        u8 port_type;
        u8 color_space;
        BEField<u16> latency;
        u8 n_available_modes;
        u8 state;
        u8 rgb_output_range;
        u8 reserved[5];
        CellVideoOutColorInfo color_info;
        CellVideoOutDisplayMode available_modes[32];
        // ksv list
    };

    std::unordered_map<u32, CellVideoOutResolutionLE> resolutions {
        { CELL_VIDEO_OUT_RESOLUTION_1080,                            { 0x780, 0x438 } },
        { CELL_VIDEO_OUT_RESOLUTION_720,                             { 0x500, 0x2d0 } },
        { CELL_VIDEO_OUT_RESOLUTION_480,                             { 0x2d0, 0x1e0 } },
        { CELL_VIDEO_OUT_RESOLUTION_576,                             { 0x2d0, 0x240 } },
        { CELL_VIDEO_OUT_RESOLUTION_1600x1080,                       { 0x640, 0x438 } },
        { CELL_VIDEO_OUT_RESOLUTION_1440x1080,                       { 0x5a0, 0x438 } },
        { CELL_VIDEO_OUT_RESOLUTION_1280x1080,                       { 0x500, 0x438 } },
        { CELL_VIDEO_OUT_RESOLUTION_960x1080,                        { 0x3c0, 0x438 } },
        { 0x64,                                                      { 0x550, 0x300 } },
        { CELL_VIDEO_OUT_RESOLUTION_720_3D_FRAME_PACKING,            { 0x500, 0x5be } },
        { 0x82,                                                      { 0x780, 0x438 } },
        { 0x83,                                                      { 0x780, 0x89d } },
        { CELL_VIDEO_OUT_RESOLUTION_640x720_3D_FRAME_PACKING,        { 0x280, 0x5be } },
        { CELL_VIDEO_OUT_RESOLUTION_800x720_3D_FRAME_PACKING,        { 0x320, 0x5be } },
        { CELL_VIDEO_OUT_RESOLUTION_960x720_3D_FRAME_PACKING,        { 0x3c0, 0x5be } },
        { CELL_VIDEO_OUT_RESOLUTION_1024x720_3D_FRAME_PACKING,       { 0x400, 0x5be } },
        { CELL_VIDEO_OUT_RESOLUTION_720_DUALVIEW_FRAME_PACKING,      { 0x500, 0x5be } },
        { 0x92,                                                      { 0x780, 0x438 } },
        { CELL_VIDEO_OUT_RESOLUTION_640x720_DUALVIEW_FRAME_PACKING,  { 0x280, 0x5be } },
        { CELL_VIDEO_OUT_RESOLUTION_800x720_DUALVIEW_FRAME_PACKING,  { 0x320, 0x5be } },
        { CELL_VIDEO_OUT_RESOLUTION_960x720_DUALVIEW_FRAME_PACKING,  { 0x3c0, 0x5be } },
        { CELL_VIDEO_OUT_RESOLUTION_1024x720_DUALVIEW_FRAME_PACKING, { 0x400, 0x5be } },
        { 0xa1,                                                      { 0x780, 0x438 } },
    };

    CellVideoOutResolutionLE getResolution();

    u64 cellVideoOutConfigure();
    u64 cellVideoOutGetConfiguration();
    u64 cellVideoOutGetDeviceInfo();
    u64 cellVideoOutGetNumberOfDevice();
    u64 cellVideoOutGetState();
    u64 cellVideoOutGetResolutionAvailability();
    u64 cellVideoOutGetResolution();
    u64 cellVideoOutGetScreenSize();

private:
    MAKE_LOG_FUNCTION(log, cellVideoOut);
};
