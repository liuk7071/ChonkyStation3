#include <Syscall.hpp>
#include "PlayStation3.hpp"


MAKE_LOG_FUNCTION(log_sys_mmapper, sys_mmapper);

static u32 next_address_alloc = 0x30000000; // TODO: store this addr somewhere instead of hardcoding it

u64 Syscall::sys_mmapper_allocate_address() {
    const u64 size = ARG0;
    const u64 flags = ARG1;
    const u64 alignment = ARG2;
    const u32 addr_ptr = ARG3;
    log_sys_mmapper("sys_mmapper_allocate_address(size: 0x%016llx, flags: 0x%016llx, alignment: 0x%016llx, addr_ptr: 0x%08x)\n", size, flags, alignment, addr_ptr);

    while (ps3->mem.isMapped(next_address_alloc).first)
        next_address_alloc += 256_MB;
    ps3->mem.write<u32>(addr_ptr, next_address_alloc);
    next_address_alloc += 256_MB;

    return CELL_OK;
}

u64 Syscall::sys_mmapper_free_shared_memory() {
    const u32 handle = ARG0;
    log_sys_mmapper("sys_mmapper_free_shared_memory(handle: %d)\n", handle);
    if (!handle) return CELL_OK;
    Helpers::debugAssert(handle, "sys_mmapper_free_shared_memory: handle is 0\n");
    
    auto block = ps3->mem.findBlockWithHandle(handle);
    if (!block.first) return CELL_OK;
    Helpers::debugAssert(block.first, "sys_mmapper_free_shared_memory: tried to free unmapped memory\n");
    ps3->mem.freePhys(block.second);

    return CELL_OK;
}

u64 Syscall::sys_mmapper_allocate_shared_memory() {
    const u64 ipc_key = ARG0;
    const u64 size = ARG1;
    const u64 flags = ARG2;
    const u32 handle_ptr = ARG3;
    log_sys_mmapper("sys_mmapper_allocate_shared_memory(ipc_key: 0x%016llx, size: %d, flags: 0x%016llx, handle_ptr: 0x%08x)\n", ipc_key, size, flags, handle_ptr);

    // Check if we're out of memory
    if (!ps3->mem.canAlloc(size)) {
        log_sys_mmapper("Out of memory! Returning error\n");
        return CELL_ENOMEM;
    }

    auto block = ps3->mem.allocPhys(size);
    auto handle = ps3->handle_manager.request();
    block->handle = handle;
    ps3->mem.write<u32>(handle_ptr, handle);

    return CELL_OK;
}

u64 Syscall::sys_mmapper_map_shared_memory() {
    const u32 addr = ARG0;
    const u32 handle = ARG1;
    const u64 flags = ARG2;
    log_sys_mmapper("sys_mmapper_map_shared_memory(addr: 0x%08x, handle: %d, flags: 0x%016llx)", addr, handle, flags);

    auto block = ps3->mem.findBlockWithHandle(handle);
    Helpers::debugAssert(handle && block.first, "sysMMapperMapMemory: unknown handle\n");
    if (ps3->mem.isMapped(addr).first) {
        log_sys_mmapperNoPrefix(" [already mapped, freeing block]\n");
        // If this area was already mapped, we free the new block. Should be OK. If things break try removing this.
        ps3->mem.freeBlockWithHandle(handle);
    }
    else {
        log_sys_mmapperNoPrefix("\n");
        auto entry = ps3->mem.mmap(addr, block.second->start, block.second->size);
        // Assign the same handle to the map entry (will be checked by sys_mmapper_free_shared_memory)
        entry->handle = handle;
    }

    return CELL_OK;
}

u64 Syscall::sys_mmapper_unmap_shared_memory() {
    const u32 addr = ARG0;
    const u32 handle_ptr = ARG1;
    log_sys_mmapper("sys_mmapper_unmap_shared_memory(addr: 0x%08x, handle_ptr: 0x%08x)\n", addr, handle_ptr);

    auto info = ps3->mem.isMapped(addr);
    Helpers::debugAssert(info.first, "sys_mmapper_unmap_shared_memory: addr is unmapped\n");

    // Unmap the entry
    ps3->mem.unmap(info.second->vaddr);

    // Give back the handle of the physical block
    ps3->mem.write<u32>(handle_ptr, info.second->handle);

    return CELL_OK;
}

u64 Syscall::sys_mmapper_search_and_map() {
    const u32 start_addr = ARG0;
    const u32 handle = ARG1;
    const u64 flags = ARG2;
    const u32 addr_ptr = ARG3;
    log_sys_mmapper("sys_mmapper_search_and_map(start_addr: 0x%08x, handle: 0x%08x, flags: 0x%016llx, addr_ptr: 0x%08x)\n", start_addr, handle, flags, addr_ptr);
    auto block = ps3->mem.ram.findBlockWithHandle(handle);
    Helpers::debugAssert(handle && block.first, "sys_mmapper_search_and_map: unknown handle\n");

    // Find area to map block to
    u64 vaddr = ps3->mem.findNextAllocatableVaddr(block.second->size, start_addr);
    auto entry = ps3->mem.mmap(vaddr, block.second->start, block.second->size);
    // Assign the same handle to the map entry
    entry->handle = handle;

    ps3->mem.write<u32>(addr_ptr, entry->vaddr);

    return CELL_OK;
}
