#pragma once

#include <common.hpp>
#include <logger.hpp>

#include <queue>
#include <unordered_map>
#include <functional>

#include <MemoryConstants.hpp>


class Memory;

class MemoryRegion {
public:
    MemoryRegion(u64 virtual_base, u64 size, Memory& mem_manager) : mem_manager(mem_manager) {
        blocks.clear();
        map.clear();
        this->virtual_base = virtual_base;
        this->size = size;
        mem = new u8[size];
        // TODO: Figure out if games actually rely on uninitialized memory being initialized to 0.
        // RPCS3 does initialize it to 0
        std::memset(mem, 0, size);
    }

    MAKE_LOG_FUNCTION(log, memory);
    
    Memory& mem_manager;
    u8* mem;
    u64 virtual_base;   // Base address of this region in the virtual address space
    u64 size;

    struct Block {
        u64 start;
        size_t size;
        u64 handle;
    };
    std::vector<Block> blocks;

    struct MapEntry {
        u64 vaddr;
        u64 paddr;
        size_t size;
        u64 handle;
    };
    std::vector<MapEntry> map;

    Block* allocPhys(size_t size);
    MapEntry* alloc(size_t size, u64 start_addr = 0);
    void free(MapEntry* entry);
    std::pair<bool, Block*> findBlockFromAddr(u64 paddr);
    std::pair<bool, Block*> findNextBlock(u64 start_addr);
    std::pair<bool, Block*> findBlockWithHandle(u64 handle);
    void freeBlockWithHandle(u64 handle);
    std::pair<bool, MapEntry*> findNextMappedArea(u64 start_addr);
    u64 findNextAllocatableVaddr(size_t size, u64 start_addr = 0);
    std::pair<bool, MapEntry*> findMapEntryWithHandle(u64 handle);
    std::pair<bool, MapEntry*> isMapped(u64 vaddr);
    MapEntry* mmap(u64 vaddr, u64 paddr, size_t size);
    void unmap(u64 vaddr);
    u64 translateAddr(u64 vaddr);
    static inline u64 pageAlign(u64 addr) { return (addr + PAGE_SIZE - 1) & ~PAGE_MASK; }

    u8* getPtrPhys(u64 paddr);
    u64 getAvailableMem();

    void printAddressMap() {
        for (auto& i : map) {
            printf("0x%016llx -> 0x%016llx\n", i.vaddr, i.vaddr + i.size - 1);
        }
    }
};

class Memory {
public:
    Memory() {
        read_table.resize(PAGE_COUNT, 0);
        write_table.resize(PAGE_COUNT, 0);
    }

    // I don't explicitly check anywhere, but it is assumed that memory regions don't overlap.
    // Just be careful when creating them
    MemoryRegion ram    = MemoryRegion(RAM_START,           RAM_SIZE,           *this);
    MemoryRegion rsx    = MemoryRegion(RSX_VIDEO_MEM_START, RSX_VIDEO_MEM_SIZE, *this);
    MemoryRegion stack  = MemoryRegion(STACK_REGION_START,  STACK_REGION_SIZE,  *this);
    MemoryRegion spu    = MemoryRegion(SPU_MEM_START,       SPU_MEM_SIZE,       *this);
    std::vector<MemoryRegion*> regions = { &ram, &rsx, &stack, &spu };

    std::pair<u64, u8*> addrToOffsetInMemory(u64 vaddr);
    u8* getPtr(u64 vaddr);

    std::vector<u8*> read_table;
    std::vector<u8*> write_table;
    void markAsFastMem(u64 page, u8* ptr, bool r, bool w);
    void markAsSlowMem(u64 page, bool r, bool w);

    MemoryRegion::Block* allocPhys(size_t size) { return ram.allocPhys(size); }
    MemoryRegion::MapEntry* alloc(size_t size, u64 start_addr = 0) { return ram.alloc(size, start_addr); }
    void free(MemoryRegion::MapEntry* entry) { ram.free(entry); }
    std::pair<bool, MemoryRegion::Block*> findBlockFromAddr(u64 paddr) { return ram.findBlockFromAddr(paddr); }
    std::pair<bool, MemoryRegion::Block*> findNextBlock(u64 start_addr) { return ram.findNextBlock(start_addr); }
    std::pair<bool, MemoryRegion::Block*> findBlockWithHandle(u64 handle) { return ram.findBlockWithHandle(handle); }
    void freeBlockWithHandle(u64 handle) { ram.freeBlockWithHandle(handle); }
    std::pair<bool, MemoryRegion::MapEntry*> findNextMappedArea(u64 start_addr) { return ram.findNextMappedArea(start_addr); }
    u64 findNextAllocatableVaddr(size_t size, u64 start_addr = 0) { return ram.findNextAllocatableVaddr(size, start_addr); }
    std::pair<bool, MemoryRegion::MapEntry*> findMapEntryWithHandle(u64 handle) { return ram.findMapEntryWithHandle(handle); }
    std::pair<bool, MemoryRegion::MapEntry*> isMapped(u64 vaddr) { return ram.isMapped(vaddr); }
    MemoryRegion::MapEntry* mmap(u64 vaddr, u64 paddr, size_t size) { return ram.mmap(vaddr, paddr, size); }
    void unmap(u64 vaddr) { ram.unmap(vaddr); }
    u64 translateAddr(u64 vaddr) { return ram.translateAddr(vaddr); }

    template<typename T> T read(u64 vaddr);
    template<typename T> void write(u64 vaddr, T data);

    // Memory watchpoints
    // Call a function when an address is read or written.
    // In case of writes, the function is called after the write.
    // In order for the function to be called, the memory page the address is part of must not be in fastmem.
    std::unordered_map<u64, std::function<void(void)>> watchpoints_r;
    std::unordered_map<u64, std::function<void(void)>> watchpoints_w;
};