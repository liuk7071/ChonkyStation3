#include "SceNpTrophy.hpp"
#include "PlayStation3.hpp"


u64 SceNpTrophy::sceNpTrophyRegisterContext() {
    const u32 ctx = ARG0;
    const u32 handle = ARG1;
    const u32 callback_ptr = ARG2;
    const u32 arg = ARG3;   // to pass to callback
    const u32 options = ARG4;
    log("sceNpTrophyRegisterContext(ctx: 0x%0x8, handle: 0x%08x, callback_ptr: 0x%08x, arg: 0x%08x, options: 0x%08x)\n", ctx, handle, callback_ptr, arg, options);

    // Call callback
    const PPUTypes::State old_state = ps3->ppu->state;
    ps3->ppu->state.gprs[3] = ctx;
    ps3->ppu->state.gprs[4] = 3;    // Installed
    ps3->ppu->state.gprs[5] = 0;
    ps3->ppu->state.gprs[6] = 0;
    ps3->ppu->state.gprs[7] = arg;
    ps3->ppu->runFunc(ps3->mem.read<u32>(callback_ptr), ps3->mem.read<u32>(callback_ptr + 4));
    
    for (int i = 0; i <= 3; i++) {
        ps3->ppu->state.gprs[3] = ctx;
        ps3->ppu->state.gprs[4] = 5;    // Setup
        ps3->ppu->state.gprs[5] = i;
        ps3->ppu->state.gprs[6] = 3;
        ps3->ppu->state.gprs[7] = arg;
        ps3->ppu->runFunc(ps3->mem.read<u32>(callback_ptr), ps3->mem.read<u32>(callback_ptr + 4));
    }

    for (int i = 0; i < 32; i++) {
        ps3->ppu->state.gprs[3] = ctx;
        ps3->ppu->state.gprs[4] = 6;    // Progress
        ps3->ppu->state.gprs[5] = i;
        ps3->ppu->state.gprs[6] = 31;
        ps3->ppu->state.gprs[7] = arg;
        ps3->ppu->runFunc(ps3->mem.read<u32>(callback_ptr), ps3->mem.read<u32>(callback_ptr + 4));
    }
    
    for (int i = 0; i < 6; i++) {
        ps3->ppu->state.gprs[3] = ctx;
        ps3->ppu->state.gprs[4] = 7;    // Finalize
        ps3->ppu->state.gprs[5] = i;
        ps3->ppu->state.gprs[6] = 5;
        ps3->ppu->state.gprs[7] = arg;
        ps3->ppu->runFunc(ps3->mem.read<u32>(callback_ptr), ps3->mem.read<u32>(callback_ptr + 4));
    }

    ps3->ppu->state.gprs[3] = ctx;
    ps3->ppu->state.gprs[4] = 8;    // Complete
    ps3->ppu->state.gprs[5] = 0;
    ps3->ppu->state.gprs[6] = 0;
    ps3->ppu->state.gprs[7] = arg;
    ps3->ppu->runFunc(ps3->mem.read<u32>(callback_ptr), ps3->mem.read<u32>(callback_ptr + 4));
    ps3->ppu->state = old_state;

    return CELL_OK;
}

u64 SceNpTrophy::sceNpTrophyCreateHandle() {
    const u32 handle_ptr = ARG0;
    log("sceNpTrophyCreateHandle(handle_ptr: 0x%08x)\n");

    ps3->mem.write<u32>(handle_ptr, ps3->handle_manager.request());

    return CELL_OK;
}

u64 SceNpTrophy::sceNpTrophyGetRequiredDiskSpace() {
    const u32 ctx = ARG0;
    const u32 handle = ARG1;
    const u32 req_space_ptr = ARG2;
    const u64 options = ARG3;
    log("sceNpTrophyGetRequiredDiskSpace(ctx: 0x%08x, handle: 0x%08x, req_space_ptr: 0x%08x, options: 0x%08x) STUBBED\n");

    ps3->mem.write<u32>(req_space_ptr, 0_KB);

    return CELL_OK;
}

u64 SceNpTrophy::sceNpTrophyGetGameInfo() {
    const u32 ctx = ARG0;
    const u32 handle = ARG1;
    const u32 details_ptr = ARG2;
    const u32 data_ptr = ARG3;
    log("sceNpTrophyGetGameInfo(ctx: 0x%08x, handle: 0x%08x, details_ptr: 0x%08x, data_ptr: 0x%08x)\n UNIMPLEMENTED\n");

    return CELL_OK;
}

u64 SceNpTrophy::sceNpTrophyGetTrophyUnlockState() {
    const u32 ctx = ARG0;
    const u32 handle = ARG1;
    const u32 flags_ptr = ARG2;
    const u32 count_ptr = ARG3;
    log("sceNpTrophyGetTrophyUnlockState(ctx: 0x%08x, handle: 0x%08x, flags_ptr: 0x%08x, count_ptr: 0x%08x) STUBBED\n");

    ps3->mem.write<u32>(count_ptr, 32);
    ps3->mem.write<u8>(flags_ptr, 0x80);

    return CELL_OK;
}

u64 SceNpTrophy::sceNpTrophyCreateContext() {
    const u32 ctx_ptr = ARG0;
    const u32 comm_id_ptr = ARG1;
    const u32 comm_sign_ptr = ARG2;
    const u64 options = ARG3;
    log("sceNpTrophyCreateContext(ctx_ptr: 0x%08x, comm_id_ptr: 0x%08x, comm_sign_ptr: 0x%08x, options: 0x%016x)\n", ctx_ptr, comm_id_ptr, comm_sign_ptr, options);

    ps3->mem.write<u32>(ctx_ptr, ps3->handle_manager.request());

    return CELL_OK;
}
