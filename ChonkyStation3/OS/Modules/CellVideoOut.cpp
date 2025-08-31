#include "CellVideoOut.hpp"
#include "PlayStation3.hpp"


CellVideoOut::CellVideoOutResolutionLE CellVideoOut::getResolution() {
    return resolutions[video_config.res_id];
}

u64 CellVideoOut::cellVideoOutConfigure() {
    const u32 video_out = ARG0;
    const u32 config_ptr = ARG1;
    const u32 option_ptr = ARG2;
    const u32 wait_for_event = ARG3;
    log("cellVideoOutConfigure(video_out: 0x%08x, config_ptr: 0x%08x, option_ptr: 0x%08x, wait_for_event: 0x%08x)\n", video_out, config_ptr, option_ptr, wait_for_event);

    CellVideoOutConfiguration* config = (CellVideoOutConfiguration*)ps3->mem.getPtr(config_ptr);
    video_config.res_id = config->res_id;
    video_config.format = config->format;
    video_config.aspect = config->aspect;
    video_config.pitch = config->pitch;

    return CELL_OK;
}

u64 CellVideoOut::cellVideoOutGetConfiguration() {
    const u32 video_out = ARG0;
    const u32 config_ptr = ARG1;
    const u32 option_ptr = ARG2;
    log("cellVideoOutGetConfiguration(video_out: 0x%08x, config_ptr: 0x%08x, option_ptr: 0x%08x)\n", video_out, config_ptr, option_ptr);

    CellVideoOutConfiguration* config = (CellVideoOutConfiguration*)ps3->mem.getPtr(config_ptr);
    config->res_id = video_config.res_id;
    config->format = video_config.format;
    config->aspect = video_config.aspect;
    config->pitch = video_config.pitch;

    return CELL_OK;
}

u64 CellVideoOut::cellVideoOutGetDeviceInfo() {
    const u32 video_out = ARG0;
    const u32 device_idx = ARG1;
    const u32 info_ptr = ARG2;
    log("cellVideoOutGetDeviceInfo(video_out: 0x%08x, device_idx: %d, info_ptr: 0x%08x)\n", video_out, device_idx, info_ptr);

    CellVideoOutDeviceInfo* info = (CellVideoOutDeviceInfo*)ps3->mem.getPtr(info_ptr);
    info->port_type = CELL_VIDEO_OUT_PORT_HDMI;
    info->state = CELL_VIDEO_OUT_OUTPUT_STATE_ENABLED;
    info->rgb_output_range = 1;
    info->n_available_modes = 1;
    info->available_modes[0].res_id = CELL_VIDEO_OUT_RESOLUTION_720;
    info->available_modes[0].scan_mode = CELL_VIDEO_OUT_SCAN_MODE_PROGRESSIVE;
    info->available_modes[0].conversion = CELL_VIDEO_OUT_DISPLAY_CONVERSION_NONE;
    info->available_modes[0].aspect = CELL_VIDEO_OUT_ASPECT_16_9;
    info->available_modes[0].refresh_rates = CELL_VIDEO_OUT_REFRESH_RATE_59_94HZ;

    return CELL_OK;
}

u64 CellVideoOut::cellVideoOutGetNumberOfDevice() {
    const u32 video_out = ARG0;
    log("cellVideoOutGetNumberOfDevice(video_out: 0x%08x)\n", video_out);

    return 1;
}

u64 CellVideoOut::cellVideoOutGetState() {
    const u32 video_out = ARG0;
    const u32 device_idx = ARG1;
    const u32 state_ptr = ARG2;
    log("cellVideoOutGetState(video_out: 0x%08x, device_idx: 0x%08x, state_ptr: 0x%08x)\n", video_out, device_idx, state_ptr);

    CellVideoOutState* state = (CellVideoOutState*)ps3->mem.getPtr(state_ptr);
    state->state = CELL_VIDEO_OUT_OUTPUT_STATE_ENABLED;
    state->color_space = CELL_VIDEO_OUT_COLOR_SPACE_RGB;
    state->display_mode.res_id = video_config.res_id;
    state->display_mode.scan_mode = CELL_VIDEO_OUT_SCAN_MODE_PROGRESSIVE;
    state->display_mode.conversion = CELL_VIDEO_OUT_DISPLAY_CONVERSION_NONE;
    // TODO: If the aspect ratio mode is set to AUTO, we are supposed to figure out what the correct aspect ratio is
    // from the given resolution. For now just pretend everything is 16:9
    state->display_mode.aspect = (video_config.aspect != CELL_VIDEO_OUT_ASPECT_AUTO) ? video_config.aspect : CELL_VIDEO_OUT_ASPECT_16_9;
    state->display_mode.refresh_rates = CELL_VIDEO_OUT_REFRESH_RATE_59_94HZ;

    return CELL_OK;
}

u64 CellVideoOut::cellVideoOutGetResolutionAvailability() {
    const u32 video_out = ARG0;
    const u32 res_id = ARG1;
    const u32 aspect = ARG2;
    const u32 option = ARG3;
    log("cellVideoOutGetResolutionAvailability(video_out: 0x%08x, res_id: 0x%08x, aspect: 0x%08x, option: 0x%08x) STUBBED\n", video_out, res_id, aspect, option);

    // TODO: implement this
    // 1 means resolution is available
    return 1;
}

u64 CellVideoOut::cellVideoOutGetResolution() {
    u32 res_id = ARG0;
    const u32 res_ptr = ARG1;
    log("cellVideoOutGetResolution(res_id: 0x%08x, res_ptr: 0x%08x)", res_id, res_ptr);

    CellVideoOutResolution* res = (CellVideoOutResolution*)ps3->mem.getPtr(res_ptr);
    CellVideoOutResolutionLE res_;
    if (resolutions.find(res_id) == resolutions.end()) {
        logNoPrefix(" WARNING: Unknown resolution ID (returning 720p)");
        res_.width = 1280;
        res_.height = 720;
        //res_.width = 0;
        //res_.height = 0;
    } else 
        res_ = resolutions[res_id];
    logNoPrefix(" [resolution: %dx%d]\n", res_.width, res_.height);

    res->width = res_.width;
    res->height = res_.height;

    return CELL_OK;
}

u64 CellVideoOut::cellVideoOutGetScreenSize() {
    const u32 video_out = ARG0;
    const u32 size_ptr = ARG1;  // size is float
    log("cellVideoOutGetScreenSize(video_out: 0x%08x, size_ptr: 0x%08x)", video_out, size_ptr);

    // This function returns the length of the diagonal of the screen.
    // For some weird reason it only works for 3D TVs, otherwise the error below is returned

    return 0x8002b228; // CELL_VIDEO_OUT_ERROR_VALUE_IS_NOT_SET
}
