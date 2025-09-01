#include "CellPad.hpp"
#include "PlayStation3.hpp"


u64 CellPad::cellPadInit() {
    const u32 max_connect = ARG0;
    log("cellPadInit(max_connect: %d)\n", max_connect);

    return CELL_OK;
}

u64 CellPad::cellPadGetInfo() {
    const u32 info_ptr = ARG0;
    log("cellPadGetInfo(info_ptr: 0x%08x)\n", info_ptr);

    CellPadInfo* info = (CellPadInfo*)ps3->mem.getPtr(info_ptr);
    info->max_connect = 4;
    info->now_connect = 1;
    info->system_info = 0;
    info->product_id[0] = 0x0268;
    info->vendor_id[0] = 0x054c;
    info->status[0] = CELL_PAD_STATUS_CONNECTED;

    return CELL_OK;
}

u64 CellPad::cellPadGetData() {
    const u32 port_num = ARG0;
    const u32 data_ptr = ARG1;
    //log("cellPadGetData(port_num: %d, data_ptr: 0x%08x) @ 0x%08x\n", port_num, data_ptr, ps3->ppu->state.lr);

    if (port_num != 0) return 0x80121107; // CELL_PAD_ERROR_NO_DEVICE

    CellPadData* data = (CellPadData*)ps3->mem.getPtr(data_ptr);
    data->len = 8;

    for (int i = 0; i < CELL_PAD_MAX_CODES; i++)
        data->button[i] = buttons[i];

    data->button[0] = 0;
    data->button[1] = ((data->len / 2) & 0xf) | (7 << 4);

    return CELL_OK;
}

u64 CellPad::cellPadGetInfo2() {
    const u32 info_ptr = ARG0;
    log("cellPadGetInfo2(info_ptr: 0x%08x)\n", info_ptr);

    CellPadInfo2* info = (CellPadInfo2*)ps3->mem.getPtr(info_ptr);
    info->max_connect = 4;
    info->now_connect = 1;
    info->system_info = 0;
    info->port_status[0] = 1;   // Connected
    info->port_setting[0] = 0;
    info->device_capability[0] = 1;
    info->device_type[0] = 0;   // Standard

    return CELL_OK;
}
