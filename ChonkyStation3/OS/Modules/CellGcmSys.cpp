#include "CellGcmSys.hpp"
#include "PlayStation3.hpp"


u64 CellGcmSys::cellGcmGetTiledPitchSize() {
    const u32 size = ARG0;
    log("cellGcmGetTiledPitchSize(size: %d)\n", size);

    for (int i = 0; i < 32; i++) {
        if (size > tiled_pitches[i] && size <= tiled_pitches[i + 1])
            return tiled_pitches[i + 1];
    }

    return 0;
}

u64 CellGcmSys::cellGcmSetQueueHandler() {
    const u32 handler_ptr = ARG0;
    log("cellGcmGetDisplayInfo(handler_ptr: 0x%08x)\n");
    
    queue_handler = handler_ptr;    
    return CELL_OK;
}

u64 CellGcmSys::cellGcmGetDisplayInfo() {
    log("cellGcmGetDisplayInfo()\n");
    return buffer_info_addr;
}

u64 CellGcmSys::cellGcmInitBody() {
    const u32 ctx_ptr = ARG0;
    const u32 cmd_size = ARG1;
    const u32 io_size = ARG2;
    const u32 io_addr = ARG3;
    log("cellGcmInitBody(ctx_ptr: 0x%08x, cmd_size: 0x%08x, io_size: 0x%08x, io_addr: 0x%08x)\n", ctx_ptr, cmd_size, io_size, io_addr);

    gcm_config.local_size = 249_MB;
    gcm_config.local_addr = ps3->mem.rsx.alloc(gcm_config.local_size)->vaddr;
    gcm_config.io_addr = io_addr;
    gcm_config.io_size = io_size;
    gcm_config.memFreq = 650000000;
    gcm_config.coreFreq = 500000000;

    std::memset(ps3->mem.getPtr(gcm_config.local_addr), 0, gcm_config.local_size);

    default_ctx_ptr = ctx_ptr;
    ctx_addr = ps3->mem.alloc(sizeof(CellGcmContextData), 0, true)->vaddr;
    CellGcmContextData* ctx = (CellGcmContextData*)ps3->mem.getPtr(ctx_addr);
    ps3->mem.write<u32>(ctx_ptr, ctx_addr);

    // Setup callback
    u32 callback_addr = ps3->mem.alloc(4 * 4, 0, true)->vaddr;
    ps3->mem.write<u32>(callback_addr +  0, callback_addr + 4);
    ps3->mem.write<u32>(callback_addr +  4, 0x39600400);     // li r11, 0x400
    ps3->mem.write<u32>(callback_addr +  8, 0x44000002);     // sc
    ps3->mem.write<u32>(callback_addr + 12, 0x4E800020);     // blr

    n_cmd_bufs = cmd_size / CMD_BUFFER_SIZE;
    log("Number of command buffers: %d\n", n_cmd_bufs);
    ctx->begin = io_addr + 4_KB;    // First 4KB of the first command buffer are reserved
    ctx->end = io_addr + CMD_BUFFER_SIZE - 4;   // -4 is for the jump command
    ctx->current = ctx->begin;
    ctx->callback = callback_addr;

    log("Initialized context\n");
    log("begin: 0x%08x, end: 0x%08x, current: 0x%08x\n", (u32)ctx->begin, (u32)ctx->end, (u32)ctx->current);
    
    dma_ctrl_addr = ps3->mem.rsx.alloc(3_MB)->vaddr;
    std::memset(ps3->mem.getPtr(dma_ctrl_addr), 0, 3_MB);
    ctrl_addr = dma_ctrl_addr + 0x40;
    ctrl = (CellGcmControl*)ps3->mem.getPtr(ctrl_addr);
    ctrl->put = 0;
    ctrl->get = 0;
    ctrl->ref = -1;

    // Allocate display buffer info
    buffer_info_addr = ps3->mem.alloc(sizeof(CellGcmDisplayInfo) * 8, 0, true)->vaddr;
    
    // Empty dummy reports area
    reports_addr = buffer_info_addr + sizeof(CellGcmDisplayInfo) * 8;

    // Memory watchpoint to tell the RSX to check if there are commands to run when put is written
    ps3->mem.watchpoints_w[ctrl_addr] = std::bind(&RSX::putWritten, &ps3->rsx, std::placeholders::_1);
    ps3->mem.markAsSlowMem(ctrl_addr >> PAGE_SHIFT, false, true);   // Only need to make writes take the slow path

    label_addr = dma_ctrl_addr + 2_MB;

    // Initialize offset table
    // The io_addr table has 3072 entries; the ea_addr table has 256 entries unless we are emulating a 512MB RSX unit.
    static constexpr size_t table_size = (3072 + 512) * sizeof(u16);
    const u32 offset_table_addr = ps3->mem.alloc(table_size, 0, true)->vaddr;
    io_table_ptr = offset_table_addr;
    ea_table_ptr = offset_table_addr + 3072 * sizeof(u16);
    std::memset(ps3->mem.getPtr(offset_table_addr), 0xff, table_size); // The table is initialized to all FFs

    for (u32 i = 0; i < (gcm_config.io_size >> 20); i++)
        mapEaIo(gcm_config.io_addr + (i << 20), i << 20);
    ps3->rsx.setEaTableAddr(ea_table_ptr);
    
    // Create vblank thread
    static const u8 code[] = {
        0x39, 0x60, 0x00, 0x8d, 0x38, 0x60, 0x03, 0xe8,
        0x44, 0x00, 0x00, 0x02, 0x39, 0x60, 0x10, 0x00,
        0x44, 0x00, 0x00, 0x02, 0x4b, 0xff, 0xff, 0xec,

    };
    u32 vblank_thread_entry = ps3->mem.alloc(1_MB, 0, true)->vaddr; 
    std::memcpy(ps3->mem.getPtr(vblank_thread_entry), code, sizeof(code));
        
    auto thread = ps3->thread_manager.createThread(vblank_thread_entry, 1, 0, 0, (const u8*)"gcm_vblank_thread", 0, 0, 0);
    thread->state.pc = vblank_thread_entry;

    // cellGcm writes some commands on init
    ctx->current = ctx->begin + 0x3a0;
    ctrl->get = 0x13a0;
    ctrl->put = 0x13a0;
    
    log("Control register addr: 0x%08x\n", ctrl_addr);
    
    return CELL_OK;
}

// Maps a 1MB page from main memory to IO address space 
void CellGcmSys::mapEaIo(u32 ea, u32 io) {
    ps3->mem.write<u16>(ea_table_ptr + ((io >> 20) * 2), ea >> 20);
    ps3->mem.write<u16>(io_table_ptr + ((ea >> 20) * 2), io >> 20);

    log("Mapped addr 0x%08x to IO offset 0x%08x\n", ea, io);
}

// Unmaps a 1MB page from main memory from IO address space 
void CellGcmSys::unmapEaIo(u32 ea, u32 io) {
    ps3->mem.write<u16>(ea_table_ptr + ((io >> 20) * 2), 0xffff);
    ps3->mem.write<u16>(io_table_ptr + ((ea >> 20) * 2), 0xffff);

    log("Unmapped addr 0x%08x from IO offset 0x%08x\n", ea, io);
}

// Converts an address to an IO offset
u32 CellGcmSys::addressToOffset(u32 addr, bool& ok) {
    u32 offs = 0;
    ok = true;
    // Check if the address is in RSX memory
    if (Helpers::inRange<u32>(addr, gcm_config.local_addr, gcm_config.local_addr + gcm_config.local_size - 1)) {
        offs = addr - gcm_config.local_addr;
    }
    // Check if it's mapped to IO
    else {
        u16* io_table = (u16*)ps3->mem.getPtr(io_table_ptr);
        const u32 page = ps3->mem.read<u16>(io_table_ptr + ((addr >> 20) * 2));
        if (page != 0xffff)
            offs = (page << 20) | (addr & 0xfffff);
        else {
            ok = false;
        }
    }
    return offs;
}

// TODO: This is broken, need to go through the read/write functions instead of using raw pointers due to endianness
void CellGcmSys::printOffsetTable() {
    u16* ea_table = (u16*)ps3->mem.getPtr(ea_table_ptr);
    u16* io_table = (u16*)ps3->mem.getPtr(io_table_ptr);

    bool logged_once = false;
    log("Offset table (ea -> io):");
    for (u32 i = 0; i < 3072; i++) {
        if (io_table[i] != 0xffff) {
            logged_once = true;
            logNoPrefix("\n");
            log("0x%08x -> 0x%08x", i << 20, io_table[i] << 20);
        }
    }
    if (!logged_once) logNoPrefix(" [empty]");
    logNoPrefix("\n");

    logged_once = false;
    log("Offset table (io -> ea):");
    for (u32 i = 0; i < 256; i++) {
        if (ea_table[i] != 0xffff) {
            logged_once = true;
            logNoPrefix("\n");
            log("0x%08x -> 0x%08x", i << 20, ea_table[i] << 20);
        }
    }
    if (!logged_once) logNoPrefix(" [empty]");
    logNoPrefix("\n");
}

// Returns whether the given offset in IO memory is mapped to an address in main memory
bool CellGcmSys::isIoOffsMapped(u32 io) {
    return ps3->mem.read<u16>(ea_table_ptr + ((io >> 20) * 2)) != 0xffff;
}

u64 CellGcmSys::_cellGcmSetFlipCommand() {
    const u32 context_addr = ARG0;
    const u32 buf_id = ARG1;
    log("_cellGcmSetFlipCommand(ctx_addr: 0x%08x, buf_id: %d)\n", context_addr, buf_id);

    CellGcmContextData* context = (CellGcmContextData*)ps3->mem.getPtr(context_addr);
    log("begin: 0x%08x, end: 0x%08x, current: 0x%08x\n", (u32)context->begin, (u32)context->end, (u32)context->current);

    if (context->current + 8 >= context->end) cellGcmCallback();
    ps3->mem.write<u32>(context->current, RSX::GCM_FLIP_COMMAND | (1 << 18));   // 1 is argc
    ps3->mem.write<u32>(context->current + 4, buf_id);
    context->current = context->current + 8;
    
    // Call queue handler
    //if (queue_handler) {
    //    ps3->ppu->state.gprs[3] = 1; // Handler function is always called with 1 as first argument
    //    ps3->ppu->runFunc(ps3->mem.read<u32>(queue_handler), ps3->mem.read<u32>(queue_handler + 4));
    //}

    return CELL_OK;
}

u64 CellGcmSys::cellGcmAddressToOffset() {
    const u32 addr = ARG0;
    const u32 offs_ptr = ARG1;
    log("cellGcmAddressToOffset(addr: 0x%08x, offs_ptr: 0x%08x) @ 0x%08x\n", addr, offs_ptr, ps3->ppu->state.lr);

    bool ok;
    const u32 offs = addressToOffset(addr, ok);
    if (!ok) {  // Tomb Raider (2013) relies on this
        log("WARNING: cellGcmAddressToOffset: addr is not in rsx memory or io memory(0x%08x)\n", addr);
        return CELL_GCM_ERROR_FAILURE;
    }
    
    log("Offset: 0x%08x\n", offs);

    //logNoPrefix(" [offs: 0x%08x]\n", offs);
    ps3->mem.write<u32>(offs_ptr, offs);
    
    return CELL_OK;
}

u64 CellGcmSys::cellGcmGetLastSecondVTime() {
    log("cellGcmGetLastSecondVTime() UNIMPLEMENTED\n");
    return CELL_OK;
}

u64 CellGcmSys::cellGcmGetOffsetTable() {
    const u32 table_ptr = ARG0;
    log("cellGcmGetOffsetTable(table_ptr: 0x%08x)\n", table_ptr);

    CellGcmOffsetTable* table = (CellGcmOffsetTable*)ps3->mem.getPtr(table_ptr);
    table->ea_addr_ptr = ea_table_ptr;
    table->io_addr_ptr = io_table_ptr;

    return CELL_OK;
}

u64 CellGcmSys::cellGcmIoOffsetToAddress() {
    const u32 io_offs = ARG0;
    const u32 addr_ptr = ARG1;
    log("cellGcmIoOffsetToAddress(io_offs: 0x%08x, addr_ptr: 0x%08x)\n", io_offs, addr_ptr);
    
    const u32 addr = ps3->rsx.ioToEa(io_offs);
    ps3->mem.write<u32>(addr_ptr, addr);
    
    return CELL_OK;
}

u64 CellGcmSys::cellGcmBindTile() {
    const u8 idx = ARG0;
    log("cellGcmBindTile(idx: %d) UNIMPLEMENTED\n", idx);

    return CELL_OK;
}

u64 CellGcmSys::cellGcmSetFlipMode() {
    const u32 mode = ARG0;
    log("cellGcmSetFlipMode(mode: 0x%08x) UNIMPLEMENTED\n", mode);

    return CELL_OK;
}

u64 CellGcmSys::cellGcmSetSecondVFrequency() {
    log("cellGcmSetSecondVFrequency() UNIMPLEMENTED\n");

    return CELL_OK;
}

u64 CellGcmSys::cellGcmSetDebugOutputLevel() {
    const s32 level = ARG0;
    log("cellGcmSetDebugOutputLevel(level: %d)\n", level);

    return CELL_OK;
}

u64 CellGcmSys::cellGcmGetDefaultCommandWordSize() {
    log("cellGcmGetDefaultCommandWordSize()\n");
    return command_size;
}

u64 CellGcmSys::cellGcmMapEaIoAddressWithFlags() {
    const u32 ea = ARG0;
    const u32 io = ARG1;
    const u32 size = ARG2;
    const u32 flags = ARG3;
    log("cellGcmMapEaIoAddressWithFlags(ea: 0x%08x, io: 0x%08x, size: 0x%08x, flags: 0x%08x)\n", ea, io, size, flags);

    const u32 n_pages = size >> 20;
    log("Mapping %d pages\n", n_pages);
    for (u32 i = 0; i < n_pages; i++)
        mapEaIo(ea + (i << 20), io + (i << 20));
    mapping_sizes[io >> 20] = n_pages;

    //printOffsetTable();
    return CELL_OK;
}

u64 CellGcmSys::cellGcmMapEaIoAddress() {
    const u32 ea = ARG0;
    const u32 io = ARG1;
    const u32 size = ARG2;
    log("cellGcmMapEaIoAddress(ea: 0x%08x, io: 0x%08x, size: 0x%08x)\n", ea, io, size);

    const u32 n_pages = size >> 20;
    log("Mapping %d pages\n", n_pages);
    for (u32 i = 0; i < n_pages; i++)
        mapEaIo(ea + (i << 20), io + (i << 20));
    mapping_sizes[io >> 20] = n_pages;

    return CELL_OK;
}

u64 CellGcmSys::cellGcmGetLastFlipTime() {
    log("cellGcmGetLastFlipTime()\n");
    return ps3->rsx.last_flip_time;
}

u64 CellGcmSys::cellGcmGetFlipStatus() {
    log("cellGcmGetFlipStatus()\n");
    return flip;
}

u64 CellGcmSys::cellGcmGetReportDataAddressLocation() {
    const u32 idx = ARG0;
    const u32 loc = ARG1;
    // TODO: We just use a random, empty location for now... will fix when I implement reports
    log("cellGcmGetReportDataAddressLocation(idx: %d, loc: %d)\n", idx, loc);

    return reports_addr;
}

u64 CellGcmSys::cellGcmGetDefaultSegmentWordSize() {
    log("cellGcmGetDefaultSegmentWordSize()\n");
    return segment_size;
}

u64 CellGcmSys::cellGcmSetWaitFlip() {
    const u32 mode = ARG0;
    log("cellGcmSetWaitFlip(mode: 0x%08x)\n", mode);

    // A NOP because we don't have a multithreaded RSX.
    // When this function is called, the RSX will have already flipped.
    //ps3->thread_manager.getCurrentThread()->sleepForCycles(CPU_FREQ - ps3->curr_block_cycles - ps3->cycle_count);
    return CELL_OK;
}

u64 CellGcmSys::cellGcmGetReportDataAddress() {
    const u32 idx = ARG0;
    log("cellGcmGetReportDataAddress(idx: %d)\n", idx);

    // TODO
    return reports_addr + idx * 16;
}

u64 CellGcmSys::cellGcmBindZcull() {
    const u8 idx = ARG0;
    const u32 offs = ARG1;
    const u32 width = ARG2;
    const u32 height = ARG3;
    const u32 cull_start = ARG4;
    const u32 z_format = ARG5;
    const u32 aa_format = ARG6;
    const u32 z_cull_dir = ARG7;
    //const u32 s_func = ARG8;
    //const u32 s_ref = ARG9;
    //const u32 s_mask = ARG10;
    log("cellGcmBindZcull() UNIMPLEMENTED\n");

    return CELL_OK;
}

u64 CellGcmSys::cellGcmMapMainMemory() {
    const u32 ea = ARG0;
    const u32 size = ARG1;
    const u32 offs_ptr = ARG2;
    log("cellGcmMapMainMemory(ea: 0x%08x, size: 0x%08x, offs_ptr: 0x%08x)\n", ea, size, offs_ptr);

    // Find an area big enough to allocate enough pages to fit size bytes
    int free_pages = 0;
    u32 offs;
    for (offs = 0; offs < (RAM_SIZE >> 20); offs++) {
        if (!isIoOffsMapped(offs << 20))
            free_pages++;
        else
            free_pages = 0;

        if (free_pages == (size >> 20)) {
            offs -= free_pages - 1;
            const u32 n_pages = size >> 20;
            log("Mapping %d pages\n", n_pages);
            for (u32 i = 0; i < n_pages; i++)
                mapEaIo(ea + (i << 20), (offs + i) << 20);
            mapping_sizes[offs] = n_pages;
            break;
        }
    }
    //printOffsetTable();
    ps3->mem.write<u32>(offs_ptr, offs << 20);

    return CELL_OK;
}

u64 CellGcmSys::cellGcmSetFlipHandler() {
    const u32 func_addr = ARG0;
    log("cellGcmSetFlipHandler(func_addr: 0x%08x)\n", func_addr);
    
    flip_callback = func_addr;
    return CELL_OK;
}

u64 CellGcmSys::cellGcmSetDisplayBuffer() {
    const u8 buf_id = ARG0;
    const u32 offs = ARG1;
    const u32 pitch = ARG2;
    const u32 width = ARG3;
    const u32 height = ARG4;
    log("cellGcmSetDisplayBuffer(buf_id: %d, offs: 0x%08x, pitch: %d, width: %d, height: %d)\n", buf_id, offs, pitch, width, height);
    Helpers::debugAssert(buf_id < 8, "cellGcmSetDisplayBuffer: invalid buf_id (%d)\n", buf_id);

    CellGcmDisplayInfo* info = (CellGcmDisplayInfo*)ps3->mem.getPtr(buffer_info_addr + sizeof(CellGcmDisplayInfo) * buf_id);
    info->offset = offs;
    info->width = width;
    info->height = height;
    info->pitch = pitch;

    return CELL_OK;
}

u64 CellGcmSys::cellGcmGetControlRegister() {
    //log("cellGcmGetControlRegister()\n");
    return ctrl_addr;
}

u64 CellGcmSys::cellGcmSetVBlankHandler() {
    const u32 handler_ptr = ARG0;
    log("cellGcmSetVBlankHandler(handler_ptr: 0x%08x)\n", handler_ptr);

    vblank_handler = handler_ptr;
    return CELL_OK;
}

u64 CellGcmSys::cellGcmResetFlipStatus() {
    log("cellGcmResetFlipStatus()\n");

    flip = 1;
    return CELL_OK;
}

u64 CellGcmSys::cellGcmSetDefaultCommandBuffer() {
    log("cellGcmSetDefaultCommandBuffer()\n");

    ps3->mem.write<u32>(default_ctx_ptr, ctx_addr);
    return CELL_OK;
}

u64 CellGcmSys::cellGcmSetTileInfo() {
    const u8 idx = ARG0;
    const u8 location = ARG1;
    const u32 offs = ARG2;
    const u32 size = ARG3;
    const u32 pitch = ARG4;
    const u8 compression = ARG5;
    const u16 base = ARG6;
    const u8 bank = ARG7;
    log("cellGcmSetTileInfo(idx: %d, location: %d, offs: 0x%08x, size: 0x%08x, pitch: 0x%08x, compression: %d, base: 0x%04x, bank: %d) UNIMPLEMENTED\n", idx, location, offs, size, pitch, compression, base, bank);

    return CELL_OK;
}

u64 CellGcmSys::cellGcmInitDefaultFifoMode() {
    const s32 mode = ARG0;
    log("cellGcmInitDefaultFifoMode(mode: %d)\n", mode);

    // TODO
    return CELL_OK;
}

u64 CellGcmSys::cellGcmSetGraphicsHandler() {
    log("cellGcmSetGraphicsHandler() UNIMPLEMENTED\n");

    // TODO
    return CELL_OK;
}

u64 CellGcmSys::cellGcmSetTile() {
    log("cellGcmSetTile() UNIMPLEMENTED\n");

    // TODO
    return CELL_OK;
}

u64 CellGcmSys::cellGcmSetZcull() {
    log("cellGcmSetZcull() UNIMPLEMENTED\n");

    // TODO
    return CELL_OK;
}

u64 CellGcmSys::cellGcmUnbindTile() {
    log("cellGcmUnbindTile() UNIMPLEMENTED\n");

    // TODO
    return CELL_OK;
}

u64 CellGcmSys::cellGcmUnmapIoAddress() {
    const u32 io = ARG0;
    log("cellGcmUnmapIoAddress(io: 0x%08x)\n", io);

    u16* ea_table = (u16*)ps3->mem.getPtr(ea_table_ptr);
    u16* io_table = (u16*)ps3->mem.getPtr(io_table_ptr);
    const u32 ea = ea_table[io >> 20];
    const u32 n_pages = mapping_sizes[io >> 20];

    Helpers::debugAssert(ea != 0xffff, "cellGcmUnmapIoAddress: offset 0x%08x is not mapped IO memory\n", io);

    log("Unmapping %d pages\n", n_pages);
    for (u32 i = 0; i < n_pages; i++)
        unmapEaIo((ea + i) << 20, io + (i << 20));
    mapping_sizes[io >> 20] = 0;

    return CELL_OK;
}

u64 CellGcmSys::cellGcmSetFlip() {
    const u32 context_addr = ARG0;
    const u32 buf_id = ARG1;
    log("cellGcmSetFlip(ctx_addr: 0x%08x, buf_id: %d)\n", context_addr, buf_id);

    CellGcmContextData* context = (CellGcmContextData*)ps3->mem.getPtr(context_addr);
    if (context->current + 8 >= context->end) cellGcmCallback();
    ps3->mem.write<u32>(context->current, RSX::GCM_FLIP_COMMAND | (1 << 18));   // 1 is argc
    ps3->mem.write<u32>(context->current + 4, buf_id);
    context->current = context->current + 8;
    /*if (context_addr == ctx_addr) {
        ctrl->put = ctrl->put + 8;
        ps3->rsx.runCommandList();
    }*/

    return CELL_OK;
}

u64 CellGcmSys::cellGcmSetSecondVHandler() {
    const u32 handler_ptr = ARG0;
    log("cellGcmSetSecondVHandler(handler_ptr: 0x%08x)\n", handler_ptr);

    vblank2_handler = handler_ptr;
    return CELL_OK;
}

u64 CellGcmSys::cellGcmGetConfiguration() {
    const u32 config_ptr = ARG0;
    log("cellGcmGetConfiguration(config_ptr: 0x%08x)\n", config_ptr);
    
    CellGcmConfig* config = (CellGcmConfig*)ps3->mem.getPtr(config_ptr);
    config->local_addr = gcm_config.local_addr;
    config->io_addr = gcm_config.io_addr;
    config->local_size = gcm_config.local_size;
    config->io_size = gcm_config.io_size;
    config->memFreq = gcm_config.memFreq;
    config->coreFreq = gcm_config.coreFreq;

    log("Got config:\n");
    log("ptr: 0x%08x\n", config_ptr);
    log("local_addr: 0x%08x\n", (u32)config->local_addr);
    log("io_addr   : 0x%08x\n", (u32)config->io_addr);
    log("local_size: 0x%08x\n", (u32)config->local_size);
    log("io_size   : 0x%08x\n", (u32)config->io_size);

    return CELL_OK;
}

u64 CellGcmSys::cellGcmUnmapEaIoAddress() {
    const u32 ea = ARG0;
    log("cellGcmUnmapEaIoAddress(ea: 0x%08x)\n", ea);

    u16* io_table = (u16*)ps3->mem.getPtr(io_table_ptr);
    const u32 io = io_table[ea >> 20];
    const u32 n_pages = mapping_sizes[io >> 20];

    // GTA San Andreas relies on this
    if (io == 0xffff) {
        log("Warning: address not mapped to IO memory\n");
        return 0x802100ff;  // CELL_GCM_ERROR_FAILURE
    }

    log("Unmapping %d pages\n", n_pages);
    for (u32 i = 0; i < n_pages; i++)
        unmapEaIo((ea + i) << 20, io + (i << 20));
    mapping_sizes[io >> 20] = 0;

    return CELL_OK;
}

u64 CellGcmSys::cellGcmGetLabelAddress() {
    const u8 idx = ARG0;
    log("cellGcmGetLabelAddress(idx: 0x%02x)\n", idx);

    return label_addr + 0x10 * idx;
}

u64 CellGcmSys::cellGcmSetVBlankFrequency() {
    log("cellGcmSetVBlankFrequency() UNIMPLEMENTED\n");

    return CELL_OK;
}


// Resets the command buffer
// If there are any remaining commands to be executed, execute them
// Update context and fifo control accordingly
u64 CellGcmSys::cellGcmCallback() {
    const u32 ctx_ptr = ARG0;
    log("cellGcmCallback(ctx_ptr: 0x%08x) @ 0x%08x\n", ctx_ptr, ps3->ppu->state.lr);
    CellGcmContextData* ctx = (CellGcmContextData*)ps3->mem.getPtr(ctx_ptr);
    log("begin: 0x%08x, end: 0x%08x, current: 0x%08x\n", (u32)ctx->begin, (u32)ctx->end, (u32)ctx->current);
    log("get: 0x%08x, put: 0x%08x\n", (u32)ctrl->get, (u32)ctrl->put);

    // cellGcmCallback automatically flushes the cmd buffer
    bool ok;
    ctrl->put = addressToOffset(ctx->current, ok);
    Helpers::debugAssert(ok, "cellGcmCallback: addressToOffset error\n");
    log("Flushing RSX command buffer up to offs 0x%08x\n", (u32)ctrl->put);
    ps3->rsx.runCommandList();

    // Find the next cmd buffer
    u32 new_begin;
    u32 new_end;
    const u32 curr_cmd_buf_idx = (ctx->begin - gcm_config.io_addr) / CMD_BUFFER_SIZE;
    if (curr_cmd_buf_idx + 1 < n_cmd_bufs) {
        // We haven't reached the end of the cmd ring buffer yet, go to the next one
        new_begin = gcm_config.io_addr + (curr_cmd_buf_idx + 1) * CMD_BUFFER_SIZE;
        new_end = new_begin + CMD_BUFFER_SIZE - 4;  // -4 is for the jump command
    } else {
        // We have reached the end of the cmd ring buffer, go back to the first one
        new_begin = gcm_config.io_addr + 4_KB;  // First 4KB are reserved
        new_end = gcm_config.io_addr + CMD_BUFFER_SIZE - 4;
    }
    
    // Write jump command
    ps3->mem.write<u32>(ctx->current, 0x20000000 | addressToOffset(new_begin, ok));
    Helpers::debugAssert(ok, "cellGcmCallback: addressToOffset error in jmp command\n");
    
    // Update cmd buffer structure
    ctx->begin   = new_begin;
    ctx->end     = new_end;
    ctx->current = ctx->begin;

    log("Updated context and control:\n");
    log("begin: 0x%08x, end: 0x%08x, current: 0x%08x\n", (u32)ctx->begin, (u32)ctx->end, (u32)ctx->current);
    log("get: 0x%08x, put: 0x%08x\n", (u32)ctrl->get, (u32)ctrl->put);

    return CELL_OK;
}
