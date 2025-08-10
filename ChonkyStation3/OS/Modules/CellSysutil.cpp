#include "CellSysutil.hpp"
#include "PlayStation3.hpp"


u64 CellSysutil::cellSysutilUnregisterCallback() {
    log("cellSysutilUnregisterCallback() UNIMPLEMENTED\n");

    return CELL_OK;
}

u64 CellSysutil::cellSysutilCheckCallback() {
    log("cellSysutilCheckCallback() STUBBED\n");

    for (int i = 0; i < 3; i++) {
        if (callbacks[i].func_ptr) {
            //log("Running callback func %d @ 0x%08x\n", i, callbacks[i].func_ptr);
            //ps3->ppu->state.gprs[3] = callbacks[i].userdata_ptr;
            //ps3->ppu->runFunc(ps3->mem.read<u32>(callbacks[i].func_ptr), ps3->mem.read<u32>(callbacks[i].func_ptr + 4));
            //log("Done\n");
        }
    }

    return CELL_OK;
}

u64 CellSysutil::cellSysutilGetSystemParamInt() {
    const u32 param = ARG0;
    const u32 val_ptr = ARG1;
    log("cellSysutilGetSystemParamInt(param: 0x%08x, val_ptr: 0x%08x)\n", param, val_ptr);

    u32 val = 0;
    switch (param) {
    case CELL_SYSUTIL_SYSTEMPARAM_ID_LANG:                          val = CELL_SYSUTIL_LANG_ENGLISH_US;             break;
    case CELL_SYSUTIL_SYSTEMPARAM_ID_ENTER_BUTTON_ASSIGN:           val = CELL_SYSUTIL_ENTER_BUTTON_ASSIGN_CROSS;   break;
    case CELL_SYSUTIL_SYSTEMPARAM_ID_DATE_FORMAT:                   val = CELL_SYSUTIL_DATE_FMT_DDMMYYYY;           break;
    case CELL_SYSUTIL_SYSTEMPARAM_ID_TIME_FORMAT:                   val = CELL_SYSUTIL_TIME_FMT_CLOCK24;            break;
    case CELL_SYSUTIL_SYSTEMPARAM_ID_TIMEZONE:                      val = 0;    /* ? */                             break;
    case CELL_SYSUTIL_SYSTEMPARAM_ID_SUMMERTIME:                    val = 0;                                        break;
    case CELL_SYSUTIL_SYSTEMPARAM_ID_GAME_PARENTAL_LEVEL:           val = CELL_SYSUTIL_GAME_PARENTAL_OFF;           break;
    case CELL_SYSUTIL_SYSTEMPARAM_ID_CURRENT_USER_HAS_NP_ACCOUNT:   val = 0;    /* no account? */                   break;
    case CELL_SYSUTIL_SYSTEMPARAM_ID_CAMERA_PLFREQ:                 val = CELL_SYSUTIL_CAMERA_PLFREQ_DISABLED;      break;
    case CELL_SYSUTIL_SYSTEMPARAM_ID_PAD_RUMBLE:                    val = CELL_SYSUTIL_PAD_RUMBLE_OFF;              break;

    default: {
        log("WARNING: Tried to get bad system param int 0x%08x\n", param);  // This happens in Resistance: Fall of Man
        return CELL_SYSUTIL_ERROR_VALUE;
    }
    }

    ps3->mem.write<u32>(val_ptr, val);
    return CELL_OK;
}

u64 CellSysutil::cellSysutilGetSystemParamString() {
    const u32 param = ARG0;
    const u32 buf_ptr = ARG1;
    const u32 buf_size = ARG2;
    log("cellSysutilGetSystemParamString(param: 0x%08x, buf_ptr: 0x%08x, buf_size: %d)\n", param, buf_ptr, buf_size);

    std::string str;
    u32 size;
    switch (param) {
    case CELL_SYSUTIL_SYSTEMPARAM_ID_NICKNAME: {
        str = ps3->settings.system.nickname;
        size = CELL_SYSUTIL_SYSTEMPARAM_NICKNAME_SIZE;
        break;
    }

    case CELL_SYSUTIL_SYSTEMPARAM_ID_CURRENT_USERNAME: {
        str = ps3->settings.system.nickname;
        size = CELL_SYSUTIL_SYSTEMPARAM_CURRENT_USERNAME_SIZE;
        break;
    }

    default:
        Helpers::panic("Tried to get unimplemented system param string 0x%08x\n", param);
    }

    Helpers::debugAssert(buf_size == size, "cellSysutilGetSystemParamString: invalid buf size %d (expected %d)\n", buf_size, size);
    u8* buf = ps3->mem.getPtr(buf_ptr);
    std::memset(buf, 0, size);
    std::memcpy(buf, str.c_str(), str.length());
    return CELL_OK;
}

u64 CellSysutil::cellSysutilRegisterCallback() {
    const u32 slot = ARG0;
    const u32 func_ptr = ARG1;
    const u32 userdata_ptr = ARG2;
    log("cellSysUtilRegisterCallback(slot: %d, func_ptr: 0x%08x, userdata_ptr: 0x%08x)\n", slot, func_ptr, userdata_ptr);
    Helpers::debugAssert(slot < 4, "cellSysutilRegisterCallback(): slot is not in range 0-3\n");

    callbacks[slot] = { func_ptr, userdata_ptr };

    return CELL_OK;
}
