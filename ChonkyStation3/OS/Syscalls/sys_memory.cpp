#include <Syscall.hpp>
#include "PlayStation3.hpp"
#include <Lv2Objects/Lv2MemoryContainer.hpp>


MAKE_LOG_FUNCTION(log_sys_memory, sys_memory);

struct sys_page_attr {
    BEField<u64> attr;
    BEField<u64> access_right;
    BEField<u32> page_size;
    BEField<u32> pad;
};

u64 Syscall::sys_memory_container_create() {
    const u32 id_ptr = ARG0;
    const u64 size = ARG1;
    log_sys_memory("sys_memory_container_create(id_ptr: 0x%08x, size: 0x%016llx)\n", id_ptr, size);

    // Round down to 1MB
    u64 rounded_size = size & ~(1_MB - 1);
    if (rounded_size < 1_MB) rounded_size = 1_MB;

    Lv2MemoryContainer* container = ps3->lv2_obj.create<Lv2MemoryContainer>();
    container->create(rounded_size);
    ps3->mem.write<u32>(id_ptr, container->handle());

    return CELL_OK;
}

u64 Syscall::sys_memory_container_destroy() {
    const u32 id = ARG0;
    log_sys_memory("sys_memory_container_destroy(id: 0x%08x)\n", id);

    Lv2MemoryContainer* container = ps3->lv2_obj.get<Lv2MemoryContainer>(id);
    container->free();

    return CELL_OK;
}

u64 Syscall::sys_memory_allocate() {
    const u32 size = ARG0;
    const u32 flags = ARG1;
    const u32 alloc_ptr = ARG2;
    log_sys_memory("sys_memory_allocate(size: 0x%08x, flags: 0x%08x, alloc_ptr: 0x%08x)\n", size, flags, alloc_ptr);

    const auto alignment = flags == 0x400 ? 1_MB : 64_KB;
    
    auto block = ps3->mem.alloc(size, 0x20000000, false, alignment);
    ps3->mem.write<u32>(alloc_ptr, block->vaddr);

    return CELL_OK;
}

u64 Syscall::sys_memory_free() {
    const u32 start_addr = ARG0;
    log_sys_memory("sys_memory_free(start_addr: 0x%08x)\n", start_addr);

    auto entry = ps3->mem.isMapped(start_addr);
    Helpers::debugAssert(entry.first, "sys_memory_free: tried to free unmapped memory\n");

    ps3->mem.free(entry.second);

    return CELL_OK;
}

u64 Syscall::sys_memory_allocate_from_container() {
    const u64 size = ARG0;
    const u32 id = ARG1;
    const u64 flags = ARG2;
    const u32 alloc_ptr = ARG3;
    log_sys_memory("sys_memory_allocate_from_container(size: %lld, id: %d, flags: 0x%016llx, alloc_ptr: 0x%08x)\n", size, id, flags, alloc_ptr);
    
    // Treat this as normal alloc
    if (!ps3->mem.canAlloc(size))
        return CELL_ENOMEM;
    
    auto block = ps3->mem.alloc(size, 0x20000000);
    ps3->mem.write<u32>(alloc_ptr, block->vaddr);
    
    return CELL_OK;
}

u64 Syscall::sys_memory_get_page_attribute() {
    const u32 addr = ARG0;
    const u32 attr_ptr = ARG1;
    log_sys_memory("sys_memory_get_page_attribute(addr: 0x%08x, attr_ptr: 0x%08x)\n", addr, attr_ptr);

    if (!ps3->mem.isMapped(addr).first) // COD MW2 relies on this
        return CELL_EINVAL;
    
    sys_page_attr* attr = (sys_page_attr*)ps3->mem.getPtr(attr_ptr);
    attr->attr = 0x40000;       // SYS_MEMORY_PROT_READ_WRITE
    attr->access_right = 0xf;   // SYS_MEMORY_ACCESS_RIGHT_ANY
    attr->page_size = ((addr >> 28) == 0xd) ? 4_KB : 1_MB;
    attr->pad = 0;

    return CELL_OK;
}

u64 Syscall::sys_memory_get_user_memory_size() {
    const u64 mem_info_ptr = ARG0;
    const auto available_mem = ps3->mem.ram.getAvailableMem();
    ps3->mem.write<u32>(mem_info_ptr, RAM_SIZE - ps3->mem.ram.system_size);
    ps3->mem.write<u32>(mem_info_ptr + 4, available_mem);

    log_sys_memory("sys_memory_get_user_memory_size(mem_info_ptr: 0x%08llx) [available memory: %lldMB]\n", mem_info_ptr, available_mem / 1024 / 1024);

    return CELL_OK;
}
