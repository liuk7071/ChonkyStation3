#include "SysPrxForUser.hpp"
#include "PlayStation3.hpp"


u64 SysPrxForUser::sysProcessExit() {
    const s32 code = ARG0;
    ps3->ppu->printState();
    printf("\n---------------------------\n");
    printf(  "PROCESS EXITED WITH CODE %d\n", code);
    exit(0);
}

u64 SysPrxForUser::sysProcessAtExitSpawn() {
    log("sysProcessAtExitSpawn()\n");

    // TODO
    return Result::CELL_OK;
}

u64 SysPrxForUser::sysStrlen() {
    const u32 str = ARG0;
    log("sysStrlen(str: 0x%08x) [str: \"%s\"]\n", str, ps3->mem.getPtr(str));

    return std::strlen((const char*)ps3->mem.getPtr(str));
}

u64 SysPrxForUser::sysGetSystemTime() {
    log("sysGetSystemTime()\n");

    return std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::high_resolution_clock::now().time_since_epoch()).count();
}

u64 SysPrxForUser::sysProcess_At_ExitSpawn() {
    log("sysProcess_At_ExitSpawn()\n");

    // TODO
    return Result::CELL_OK;
}

u64 SysPrxForUser::sysSpinlockInitialize() {
    const u64 ptr = ARG0;
    log("sysSpinlockInitialize(lock_ptr: 0x%08llx)\n", ptr);

    ps3->mem.write<u32>(ptr, 0);
    return Result::CELL_OK;
}

u64 SysPrxForUser::sysProcessIsStack() {
    const u32 addr = ARG0;
    log("sys_process_is_stack(addr: 0x%08x)\n");
    return (addr >> 28) == (STACK_REGION_START >> 28);
}

u64 SysPrxForUser::sysStrcat() {
    const u32 dst = ARG0;
    const u32 src = ARG1;
    log("sysStrcat(dst: 0x%08x, src: 0x%08x)\n", dst, src);

    u32 str_start = dst;
    while (*ps3->mem.getPtr(str_start))
        str_start++;

    for (int i = 0;; i++) {
        if (!(ps3->mem.getPtr(str_start)[i] = ps3->mem.getPtr(src)[i]))
        {
            return dst;
        }
    }

    return dst;
}

u64 SysPrxForUser::sysStrncat() {
    const u32 dst = ARG0;
    const u32 src = ARG1;
    const u32 max = ARG2;
    log("sysStrcat(dst: 0x%08x, src: 0x%08x, max: %d)\n", dst, src, max);

    u32 str_start = dst;
    while (*ps3->mem.getPtr(str_start))
        str_start++;

    for (int i = 0; i < max; i++) {
        if (!(ps3->mem.getPtr(str_start)[i] = ps3->mem.getPtr(src)[i]))
        {
            return dst;
        }
    }

    ps3->mem.getPtr(str_start)[max] = '\0';
    return dst;
}

u64 SysPrxForUser::sysStrcpy() {
    const u32 dst = ARG0;
    const u32 src = ARG1;
    log("sysStrcpy(dst: 0x%08x, src: 0x%08x)", dst, src);

    std::strcpy((char*)ps3->mem.getPtr(dst), (char*)ps3->mem.getPtr(src));
    logNoPrefix(" [copied str: \"%s\"]\n", ps3->mem.getPtr(dst));
    return dst;
}

u64 SysPrxForUser::sysSpinlockLock() {
    const u64 ptr = ARG0;
    log("sysSpinlockLock(lock_ptr: 0x%08llx)\n", ptr);

    if (ps3->mem.read<u32>(ptr))
        Helpers::panic("Tried to lock already locked lock\n");

    ps3->mem.write<u32>(ptr, 1);
    return Result::CELL_OK;
}

u64 SysPrxForUser::sysSpinlockUnlock() {
    const u64 ptr = ARG0;
    log("sysSpinlockUnlock(lock_ptr: 0x%08llx)\n", ptr);

    // TODO: should I actually check for this?
    if (!ps3->mem.read<u32>(ptr))
        Helpers::panic("Tried to unlock already unlocked lock\n");

    ps3->mem.write<u32>(ptr, 0);
    return Result::CELL_OK;
}

u64 SysPrxForUser::sysMemset() {
    const u32 dst = ARG0;
    const u32 val = ARG1;
    const u32 size = ARG2;
    log("sysMemset(dst: 0x%08x, val: 0x%08x, size: 0x%08x)\n", dst, val, size);

    std::memset(ps3->mem.getPtr(dst), val, size);
    return Result::CELL_OK;
}

u64 SysPrxForUser::sysMemcpy() {
    const u32 dst = ARG0;
    const u32 src = ARG1;
    const u32 size = ARG2;
    log("sysMemcpy(dst: 0x%08x, src: 0x%08x, size: 0x%08x)\n", dst, src, size);

    std::memcpy(ps3->mem.getPtr(dst), ps3->mem.getPtr(src), size);
    return Result::CELL_OK;
}

u64 SysPrxForUser::sysMemcmp() {
    const u32 buf1 = ARG0;
    const u32 buf2 = ARG1;
    const u32 size = ARG2;
    log("sysMemcmp(dst: 0x%08x, src: 0x%08x, size: 0x%08x)\n", buf1, buf2, size);

    return std::memcmp(ps3->mem.getPtr(buf1), ps3->mem.getPtr(buf2), size);
}

u64 SysPrxForUser::sys_spu_image_import() {
    const u32 image_ptr = ARG0;
    const u32 src = ARG1;
    const u32 type = ARG2;
    log("sys_spu_image_import(image_ptr: 0x%08x, src: 0x%08x, type: 0x%08x) STUBBED\n", image_ptr, src, type);

    Syscall::SysSpuImage* image = (Syscall::SysSpuImage*)ps3->mem.getPtr(image_ptr);
    image->type = type;
    image->segs_ptr = 0xfdeadb00;
    image->n_segs = 0;
    image->entry_point = 0xfdeadb01;

    return Result::CELL_OK;
}