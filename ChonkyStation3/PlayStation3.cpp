#include "PlayStation3.hpp"

PlayStation3::PlayStation3(const fs::path& executable) : elf_parser(executable), rsx(this), syscall(this), module_manager(this), thread_manager(this), spu_thread_manager(this), prx_manager(this), fs(this), lv2_obj(this, &handle_manager) {
    createProcessors();
    
    // Load settings
    settings.load();

    module_manager.init();

    // Initialize filesystem
    fs.mount(Filesystem::Device::DEV_HDD0, settings.filesystem.dev_hdd0_mountpoint);
    fs.mount(Filesystem::Device::DEV_HDD1, settings.filesystem.dev_hdd1_mountpoint);
    fs.mount(Filesystem::Device::DEV_FLASH, settings.filesystem.dev_flash_mountpoint);
    fs.mount(Filesystem::Device::DEV_USB000, settings.filesystem.dev_usb000_mountpoint);
    fs.initialize();

    // An executable was passed as CLI argument
    if (!(executable.generic_string() == "")) {
        elf_path = executable;
    }
}

PlayStation3::~PlayStation3() {
    // Join lockline waiter threads
    for (auto& thread : spu_thread_manager.threads) {
        thread.lockline_waiter->end();
    }
    
    // Join audio thread
    module_manager.cellAudio.endAudioThread();

    // Free memory
    for (auto& i : mem.regions)
        free(i->mem);
}

void PlayStation3::gameSelector() {
    // Find installed games
    GameLoader game_loader = GameLoader(this);  // Requires dev_hdd0 to be mounted
    // Print games
    printf("Found %lld installed games:\n", game_loader.games.size());
    for (int i = 0; i < game_loader.games.size(); i++) {
        printf("%s\n", std::format("[{:>3d}] {} | {:<40s} | {:<40s}", i, game_loader.games[i].id, game_loader.games[i].title, game_loader.games[i].content_path.generic_string()).c_str());
    }
    // Ask the user what game to run
    int idx;
    printf("Enter game to run (index): ");
    std::cin >> idx;
    while (idx < 0 || idx >= game_loader.games.size()) {
        printf("Invalid index. Retry: ");
        std::cin >> idx;
    }

    loadGame(game_loader.games[idx]);
}

void PlayStation3::loadGame(const GameLoader::InstalledGame& game) {
    curr_game = game;
    // Tell cellGame the path of the game's contents
    module_manager.cellGame.setContentPath(game.content_path);
    // Get path of EBOOT.elf
    elf_path = fs.guestPathToHost(game.content_path / "USRDIR/EBOOT.elf");
    elf_path_encrypted = (game.content_path / "USRDIR/EBOOT.BIN").generic_string();
}

void PlayStation3::setFlipHandler(std::function<void(void)> const& handler) {
    flip_handler = handler;
}

void PlayStation3::init() {
    // Only init if we aren't replaying an RSX capture (aka if we actually booted something)
    if (!rsx_capture_path.empty()) return;

    // Load ELF file
    ELFLoader elf = ELFLoader(this, mem);
    std::unordered_map<u32, u32> imports = {};
    ELFLoader::PROCParam proc_param;
    auto entry = elf.load(elf_path, imports, proc_param, module_manager);

    // Mount /app_home
    fs.mount(Filesystem::Device::APP_HOME, elf_path.parent_path());

    // Register ELF module imports in module manager
    for (auto& i : imports)
        module_manager.registerImport(i.first, i.second);

    // Create main thread
    thread_manager.setTLS(elf.tls_vaddr, elf.tls_filesize, elf.tls_memsize);
    elf_path_encrypted += '\0';

    Helpers::debugAssert(!proc_param.ppc_seg, "Unhandled non-zero ppc_seg\n");
    Thread* main_thread = thread_manager.createThread(entry, proc_param.primary_stacksize, 0, proc_param.primary_prio, (const u8*)"main", elf.tls_vaddr, elf.tls_filesize, elf.tls_memsize, true, elf_path_encrypted);
    ppu->state.gprs[11] = entry;
    ppu->state.gprs[12] = proc_param.malloc_pagesize ? proc_param.malloc_pagesize : 0x100000;

    // Create the idle thread
    // This thread will run when no PPU thread is active but at least 1 SPU thread is

    // I know this is very stupid, but we have to put the idle thread code on the stack.
    // If I put it in main memory some games will abort on boot because the RAM usage exceeds their expectations (TLOU)
    // Again, I know that executing code out of the stack is very wrong. But whatever.
    const u32 idle_thread_entry = mem.stack.alloc(1_MB)->vaddr;

    // label:
    mem.write<u32>(idle_thread_entry + 0, 0x60000000);  // nop
    mem.write<u32>(idle_thread_entry + 4, 0x4bfffffc);  // b label
    mem.write<u32>(idle_thread_entry + 10, idle_thread_entry);

    Thread* idle_thread = thread_manager.createThread(idle_thread_entry + 10, DEFAULT_STACK_SIZE, 0, 0, (const u8*)"idle", 0, 0, 0);
    idle_thread->is_idle_thread = true;
    // Tell the thread manager the ID of the idle thread
    thread_manager.idle_thread_id = idle_thread->id;
    
    // "PPU return function"
    // This will be the return address for functions executed with ppu->runFunc.
    // runFunc saves the current PPU state and jumps to the function.
    // When the function returns to this address a syscall will be called which will restore the state
    ppu_ret_func = idle_thread_entry + 0x10;
    mem.write<u32>(ppu_ret_func + 0, 0x39603000);   // li r11, 0x3000
    mem.write<u32>(ppu_ret_func + 4, 0x44000002);   // sc
    
    ppu_ret_func_all_state = ppu_ret_func + 0x10;
    mem.write<u32>(ppu_ret_func_all_state + 0, 0x39603001);   // li r11, 0x3001
    mem.write<u32>(ppu_ret_func_all_state + 4, 0x44000002);   // sc
    
    prx_manager.createLv2PRXs();

    // Load PRXs required by the ELF
    prx_manager.loadModulesRecursively();

    // Initialize libraries (must be done after creating main thread)
    try {
        prx_manager.initializeLibraries();
    }
    catch (std::runtime_error e) {
        printCrashInfo(e);
    }
    
    // SPU Debugging options
    if (!settings.debug.enable_spu_after_pc.empty()) {
        enable_spu_on_pc = std::stoul(settings.debug.enable_spu_after_pc, 0, 16);
        spu_thread_to_enable = settings.debug.spu_thread_to_enable;
        mem.watchpoints_r[enable_spu_on_pc] = std::bind(&PlayStation3::enableSPUOnPC, this, std::placeholders::_1);
        mem.markAsSlowMem(enable_spu_on_pc >> PAGE_SHIFT, true, false);
        printf("Will enable SPU Thread %s on pc 0x%08x\n", spu_thread_to_enable.c_str(), enable_spu_on_pc);
    }
}

void PlayStation3::run() {
    try {
        curr_block_cycles = 0;
        while (cycle_count < CPU_FREQ)
            step();
        cycle_count = 0;
    }
    catch (std::runtime_error e) {
        printCrashInfo(e);
    }
}

static constexpr int reschedule_every_n_cycles = 512 * 2048;
void PlayStation3::step() {
    const int cycles = ppu->step();
    spu->step();

    scheduler.tick(cycles);
    curr_block_cycles += cycles;
    if (curr_block_cycles > reschedule_every_n_cycles) {
        curr_block_cycles = 0;
        thread_manager.reschedule();
    }
}

void PlayStation3::printCrashInfo(std::runtime_error err) {
    printf("PPU state:\n");
    ppu->printState();
    printf("SPU state:\n");
    spu->printState();

    const std::string error = err.what();
    printf("FATAL: %s\n", err.what());

#ifndef CHONKYSTATION3_USER_BUILD
    ppu->printCallStack();

    //for (auto& instr : crash_analyzer.state_queue) {
    //    printf("%s\n", PPUDisassembler::disasm(instr.state, instr.instr, &mem).c_str());
    //}
    
    //printf("The crash happened at the following instruction:\n");
    //printf("%s\n", PPUDisassembler::disasm(ppu->state, crash_analyzer.lastInstr(), &mem).c_str());
    //crash_analyzer.analyzeCrash(error);
#endif
    std::exit(0);
}

void PlayStation3::flip() {
    module_manager.cellGcmSys.flip = 0;
    if (module_manager.cellGcmSys.flip_callback) {
        u32 old_r3 = ppu->state.gprs[3];
        ppu->state.gprs[3] = 1; // Callback function is always called with 1 as first argument
        ppu->runFunc(mem.read<u32>(module_manager.cellGcmSys.flip_callback), mem.read<u32>(module_manager.cellGcmSys.flip_callback + 4));
        ppu->state.gprs[3] = old_r3;
    }
    mem.write<u64>(module_manager.cellGcmSys.label_addr + 0x10, 0);
    mem.write<u64>(module_manager.cellGcmSys.label_addr + 0x10 + 8, 0);

    flip_handler();
}

void PlayStation3::vblank() {
    if (module_manager.cellGcmSys.vblank_handler) {
        u32 old_r3 = ppu->state.gprs[3];
        ppu->state.gprs[3] = 1; // Handler function is always called with 1 as first argument
        ppu->runFunc(mem.read<u32>(module_manager.cellGcmSys.vblank_handler), mem.read<u32>(module_manager.cellGcmSys.vblank_handler + 4));
        ppu->state.gprs[3] = old_r3;
    }
    
    if (module_manager.cellGcmSys.vblank2_handler) {
        u32 old_r3 = ppu->state.gprs[3];
        ppu->state.gprs[3] = 1; // TODO: I don't know if second vblank is also called with 1 as argument
        ppu->runFunc(mem.read<u32>(module_manager.cellGcmSys.vblank2_handler), mem.read<u32>(module_manager.cellGcmSys.vblank2_handler + 4));
        ppu->state.gprs[3] = old_r3;
    }
}

// Returns whether or not there was a next event
bool PlayStation3::skipToNextEvent() {
    u64 ticks;
    bool ok = scheduler.tickToNextEvent(ticks);
    if (ok) {
        cycle_count += ticks;
        skipped_cycles += ticks;
    }
    return ok;
}

void PlayStation3::pressButton(u32 button) {
    int idx = 2;
    if (button >= (1 << 16)) {
        idx = 3;
        button >>= 16;
    }
    module_manager.cellPad.buttons[idx] |= button;
}

void PlayStation3::setLeftStick(float x, float y) {
    module_manager.cellPad.buttons[6] = (u8)(x * 255);
    module_manager.cellPad.buttons[7] = (u8)(y * 255);
}

void PlayStation3::setRightStick(float x, float y) {
    module_manager.cellPad.buttons[4] = (u8)(x * 255);
    module_manager.cellPad.buttons[5] = (u8)(y * 255);
}

void PlayStation3::resetButtons() {
    for (int i = 0; i < CELL_PAD_MAX_CODES; i++)
        module_manager.cellPad.buttons[i] = 0;
}

void PlayStation3::enableSPUOnPC(u32 unused) {
    if (ppu->state.pc == enable_spu_on_pc) {
        for (auto& i : spu_thread_manager.threads) {
            if (i.name == spu_thread_to_enable) {
                i.wakeUp();
                mem.markAsFastMem(enable_spu_on_pc >> PAGE_SHIFT, mem.getPtr(enable_spu_on_pc & ~PAGE_MASK), true, true);
                return;
            }
        }
    }
}

void PlayStation3::createProcessors() {
    ppu = std::make_unique<PPUInterpreter>(mem, this);
    spu = std::make_unique<SPUInterpreter>(this);
}