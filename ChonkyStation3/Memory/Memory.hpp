#pragma once

#include <common.hpp>
#include <logger.hpp>

#include <queue>
#include <unordered_map>
#include <functional>
#include <optional>
#include <mutex>

#include <MemoryConstants.hpp>

// Logs addresses which are being read without ever being written to
//#define TRACK_UNWRITTEN_READS

class Memory;

class MemoryRegion {
public:
    MemoryRegion(u64 virtual_base, u64 size, Memory& mem_manager, u64 system_size = 0) : mem_manager(mem_manager) {
        blocks.clear();
        map.clear();
        this->virtual_base = virtual_base;
        this->size = size;
        this->system_size = system_size;
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
    u64 system_size;

    struct Block {
        u64 start;
        size_t size;
        u64 handle;
        bool system;    // If the block was allocated by the OS it will not be counted when the game asks for available mem
    };
    std::vector<Block> blocks;

    struct MapEntry {
        u64 vaddr;
        u64 paddr;
        size_t size;
        u64 handle;
    };
    std::vector<MapEntry> map;

    Block* allocPhys(size_t size, bool system = false);
    MapEntry* alloc(size_t size, u64 start_addr = 0, bool system = false, u64 alignment = PAGE_SIZE);
    bool canAlloc(size_t size);
    void free(MapEntry* entry);
    void freePhys(Block* block);
    std::pair<bool, Block*> findBlockFromAddr(u64 paddr);
    std::pair<bool, Block*> findNextBlock(u64 start_addr);
    std::pair<bool, Block*> findBlockWithHandle(u64 handle);
    void freeBlockWithHandle(u64 handle);
    std::pair<bool, MapEntry*> findNextMappedArea(u64 start_addr);
    u64 findNextAllocatableVaddr(size_t size, u64 start_addr = 0, u64 alignment = PAGE_SIZE);
    std::pair<bool, MapEntry*> findMapEntryWithHandle(u64 handle);
    std::pair<bool, MapEntry*> isMapped(u64 vaddr);
    MapEntry* mmap(u64 vaddr, u64 paddr, size_t size, bool fastmem = true);
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
    
private:
    std::recursive_mutex mutex;
};

class Memory {
public:
    Memory() {
        read_table.resize(PAGE_COUNT, 0);
        write_table.resize(PAGE_COUNT, 0);
    }

    // I don't explicitly check anywhere, but it is assumed that memory regions don't overlap.
    // Just be careful when creating them
    MemoryRegion ram        = MemoryRegion(RAM_START,           RAM_SIZE,           *this, 10_MB);
    MemoryRegion rsx        = MemoryRegion(RSX_VIDEO_MEM_START, RSX_VIDEO_MEM_SIZE, *this);
    MemoryRegion stack      = MemoryRegion(STACK_REGION_START,  STACK_REGION_SIZE,  *this);
    MemoryRegion raw_spu    = MemoryRegion(RAW_SPU_MEM_START,   RAW_SPU_OFFSET * 5, *this);
    std::vector<MemoryRegion*> regions = { &ram, &rsx, &stack, &raw_spu };

    std::pair<u64, u8*> addrToOffsetInMemory(u64 vaddr);
    u8* getPtr(u64 vaddr);

    std::vector<u8*> read_table;
    std::vector<u8*> write_table;
    void markAsFastMem(u64 page, u8* ptr, bool r, bool w);
    void markAsSlowMem(u64 page, bool r, bool w);

    MemoryRegion::Block* allocPhys(size_t size) { return ram.allocPhys(size); }
    MemoryRegion::MapEntry* alloc(size_t size, u64 start_addr = 0, bool system = false, u64 alignment = PAGE_SIZE) { return ram.alloc(size, start_addr, system, alignment); }
    bool canAlloc(size_t size) { return ram.canAlloc(size); }
    void free(MemoryRegion::MapEntry* entry) { ram.free(entry); }
    void freePhys(MemoryRegion::Block* block) { ram.freePhys(block); }
    std::pair<bool, MemoryRegion::Block*> findBlockFromAddr(u64 paddr) { return ram.findBlockFromAddr(paddr); }
    std::pair<bool, MemoryRegion::Block*> findNextBlock(u64 start_addr) { return ram.findNextBlock(start_addr); }
    std::pair<bool, MemoryRegion::Block*> findBlockWithHandle(u64 handle) { return ram.findBlockWithHandle(handle); }
    void freeBlockWithHandle(u64 handle) { ram.freeBlockWithHandle(handle); }
    std::pair<bool, MemoryRegion::MapEntry*> findNextMappedArea(u64 start_addr) { return ram.findNextMappedArea(start_addr); }
    u64 findNextAllocatableVaddr(size_t size, u64 start_addr = 0, u64 alignment = PAGE_SIZE) { return ram.findNextAllocatableVaddr(size, start_addr, alignment); }
    std::pair<bool, MemoryRegion::MapEntry*> findMapEntryWithHandle(u64 handle) { return ram.findMapEntryWithHandle(handle); }
    std::pair<bool, MemoryRegion::MapEntry*> isMapped(u64 vaddr) { return ram.isMapped(vaddr); }
    MemoryRegion::MapEntry* mmap(u64 vaddr, u64 paddr, size_t size, bool fastmem = true) { return ram.mmap(vaddr, paddr, size, fastmem); }
    void unmap(u64 vaddr) { ram.unmap(vaddr); }
    u64 translateAddr(u64 vaddr) { return ram.translateAddr(vaddr); }

    // Reservations
    struct Reservation {
        u64 thread_id = 0;
        u64 vaddr = 0;
        size_t size = 0;
        std::vector<std::function<void(void)>> reservation_lost_handlers = {};
    };
    void reserveAddress(u64 vaddr);
    void reserveAddress(u64 vaddr, size_t size, u64 thread_id, std::optional<std::function<void(void)>> reservation_lost_handler = std::nullopt, bool is_spu = false);
    bool acquireReservation(u64 vaddr); // Returns false if the reservation was lost
    bool acquireReservation(u64 vaddr, u64 thread_id);
    void addReservationLostHandler(u64 vaddr, std::function<void(void)> reservation_lost_handler);
    void reservedWrite(u64 vaddr);  // Handler function for reserved writes
    void deleteReservation(u64 vaddr);
    u64 getReservationOwner(u64 vaddr);
    bool isAddressReserved(u64 vaddr);
    void setCurrentThreadID(u64 id) { curr_thread_id = id; }
    std::unordered_map<u64, Reservation> reservations;  // First u64 is the vaddr

    template<typename T> T read(u64 vaddr);
    template<typename T> void write(u64 vaddr, T data);

    // Memory watchpoints
    // Call a function when an address is read or written.
    // In case of reads, the function is called before the read.
    // In case of writes, the function is called after the write.
    // This allows the watchpoint handler to have access to the data being read or written.
    // In order for the function to be called, the memory page the address is part of must not be in fastmem.
    // For both reads and writes, the address being read/written is passed as an argument to the handler.
    std::unordered_map<u64, std::function<void(u64)>> watchpoints_r;
    std::unordered_map<u64, std::function<void(u64)>> watchpoints_w;
    
#ifdef TRACK_UNWRITTEN_READS
    std::unordered_map<u32, u64> written_addresses;
#endif
    
private:
    u64 curr_thread_id = 0;
};
