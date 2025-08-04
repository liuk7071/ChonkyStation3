#include "SysPrxForUser.hpp"
#include "PlayStation3.hpp"
#include <OS/Syscalls/sys_spu.hpp>


using namespace sys_spu;

u64 SysPrxForUser::sysProcessExit() {
    const s32 code = ARG0;
    Helpers::panic("PROCESS EXITED WITH CODE %d\n", code);
    return CELL_OK;
}

u64 SysPrxForUser::sysProcessAtExitSpawn() {
    log("sysProcessAtExitSpawn()\n");

    // TODO
    return CELL_OK;
}

u64 SysPrxForUser::sysStrlen() {
    const u32 str = ARG0;
    log("sysStrlen(str: 0x%08x) [str: \"%s\"]\n", str, ps3->mem.getPtr(str));

    return std::strlen((const char*)ps3->mem.getPtr(str));
}

u64 SysPrxForUser::sysGetSystemTime() {
    //log("sysGetSystemTime()\n");

    return std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::high_resolution_clock::now().time_since_epoch()).count();
}

u64 SysPrxForUser::sysProcess_At_ExitSpawn() {
    log("sysProcess_At_ExitSpawn()\n");

    // TODO
    return CELL_OK;
}

u64 SysPrxForUser::sysSpinlockInitialize() {
    const u64 ptr = ARG0;
    log("sysSpinlockInitialize(lock_ptr: 0x%08llx)\n", ptr);

    ps3->mem.write<u32>(ptr, 0);
    return CELL_OK;
}

u64 SysPrxForUser::sysProcessIsStack() {
    const u32 addr = ARG0;
    log("sys_process_is_stack(addr: 0x%08x)\n", addr);
    return (addr >> 28) == (STACK_REGION_START >> 28);
}

u64 SysPrxForUser::sysPrintf() {
    const u32 fmt_ptr = ARG0;
    const auto fmt = Helpers::readString(ps3->mem.getPtr(fmt_ptr));
    printf("%s", fmt.c_str());
    return CELL_OK;
}

u64 SysPrxForUser::sysStrcat() {
    const u32 dst = ARG0;
    const u32 src = ARG1;
    log("_sys_strcat(dst: 0x%08x, src: 0x%08x)\n", dst, src);

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
    log("_sys_strcpy(dst: 0x%08x, src: 0x%08x)", dst, src);

    std::strcpy((char*)ps3->mem.getPtr(dst), (char*)ps3->mem.getPtr(src));
    logNoPrefix(" [copied str: \"%s\"]\n", ps3->mem.getPtr(dst));
    return dst;
}

u64 SysPrxForUser::sysStrncpy() {
    const u32 dst = ARG0;
    const u32 src = ARG1;
    const s32 len = ARG2;
    log("_sys_strncpy(dst: 0x%08x, src: 0x%08x, len: %d)\n", dst, src, len);

    std::strncpy((char*)ps3->mem.getPtr(dst), (char*)ps3->mem.getPtr(src), len);
    return dst;
}

u64 SysPrxForUser::sysSpinlockLock() {
    const u64 ptr = ARG0;
    log("sysSpinlockLock(lock_ptr: 0x%08llx)\n", ptr);

    if (ps3->mem.read<u32>(ptr))
        Helpers::panic("Tried to lock already locked lock\n");

    ps3->mem.write<u32>(ptr, 1);
    return CELL_OK;
}

u64 SysPrxForUser::sysSpinlockUnlock() {
    const u64 ptr = ARG0;
    log("sysSpinlockUnlock(lock_ptr: 0x%08llx)\n", ptr);

    // TODO: should I actually check for this?
    if (!ps3->mem.read<u32>(ptr))
        Helpers::panic("Tried to unlock already unlocked lock\n");

    ps3->mem.write<u32>(ptr, 0);
    return CELL_OK;
}

u64 SysPrxForUser::sysMemset() {
    const u32 dst = ARG0;
    const u32 val = ARG1;
    const u32 size = ARG2;
    log("_sys_memset(dst: 0x%08x, val: 0x%08x, size: 0x%08x)\n", dst, val, size);

    std::memset(ps3->mem.getPtr(dst), val, size);
    return CELL_OK;
}

u64 SysPrxForUser::sysMemcpy() {
    const u32 dst = ARG0;
    const u32 src = ARG1;
    const u32 size = ARG2;
    log("_sys_memcpy(dst: 0x%08x, src: 0x%08x, size: 0x%08x)\n", dst, src, size);

    std::memcpy(ps3->mem.getPtr(dst), ps3->mem.getPtr(src), size);
    return CELL_OK;
}

u64 SysPrxForUser::sysMalloc() {
    const u32 size = ARG0;
    log("_sys_malloc(size: %d)\n", size);

    return ps3->mem.alloc(size)->vaddr;
}

u64 SysPrxForUser::sysFree() {
    const u32 addr = ARG0;
    log("_sys_free(addr: 0x%08x)\n", addr);

    auto [ok, entry] = ps3->mem.isMapped(addr);
    Helpers::debugAssert(ok, "_sys_free: addr 0x%08x was not mapped\n", addr);
    ps3->mem.free(entry);

    return CELL_OK;
}

u64 SysPrxForUser::sysMemcmp() {
    const u32 buf1 = ARG0;
    const u32 buf2 = ARG1;
    const u32 size = ARG2;
    log("_sys_memcmp(dst: 0x%08x, src: 0x%08x, size: 0x%08x)\n", buf1, buf2, size);

    return std::memcmp(ps3->mem.getPtr(buf1), ps3->mem.getPtr(buf2), size);
}

u64 SysPrxForUser::sys_spu_image_import() {
    const u32 image_ptr = ARG0;
    const u32 src = ARG1;
    const u32 type = ARG2;
    log("sys_spu_image_import(image_ptr: 0x%08x, src: 0x%08x, type: 0x%08x)\n", image_ptr, src, type);
    
    SPULoader loader = SPULoader(ps3);
    sys_spu_image* image = (sys_spu_image*)ps3->mem.getPtr(image_ptr);
    loader.load(src, image);

    return CELL_OK;
}