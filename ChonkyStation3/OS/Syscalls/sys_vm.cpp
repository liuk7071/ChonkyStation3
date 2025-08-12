#include <Syscall.hpp>
#include "PlayStation3.hpp"


MAKE_LOG_FUNCTION(log_sys_vm, sys_vm);

u64 Syscall::sys_vm_memory_map() {
    const u64 vsize = ARG0;
    const u64 psize = ARG1;
    const u32 ct_id = ARG2;
    const u64 flag = ARG3;
    const u64 policy = ARG4;
    const u32 addr_ptr = ARG5;
    log_sys_vm("sys_vm_memory_map(vsize: 0x%016llx, psize: 0x%016llx, ct_id: %d, flag: 0x%016llx, policy: 0x%016llx, addr_ptr: 0x%08x)\n", vsize, psize, ct_id, flag, policy, addr_ptr);

    // TODO: We just treat this as a normal alloc for now (always allocate from the "main" memory container)
    // HACK: If we can't allocate vsize, allocate psize
    const auto size = ps3->mem.canAlloc(vsize) ? psize : psize;
    auto block = ps3->mem.alloc(size, 0x20000000);
    ps3->mem.write<u32>(addr_ptr, block->vaddr);

    return CELL_OK;
}

u64 Syscall::sys_vm_append_memory() {
    const u32 addr = ARG0;
    const u64 size = ARG1;
    log_sys_vm("sys_vm_append_memory(addr: 0x%08x, size: 0x%016llx)\n", addr, size);

    // TODO: We just treat this as a normal alloc for now
    auto block = ps3->mem.allocPhys(size);
    ps3->mem.mmap(ps3->mem.findNextAllocatableVaddr(size, addr), block->start, size);

    return CELL_OK;
}

u64 Syscall::sys_vm_touch() {
    const u32 addr = ARG0;
    const u32 size = ARG1;
    log_sys_vm("sys_vm_touch(addr: 0x%08x, size: 0x%08x)\n", addr, size);

    return CELL_OK;
}
