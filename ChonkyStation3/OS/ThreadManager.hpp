#pragma once

#include <common.hpp>

#include <Thread.hpp>


// Circular dependency
class PlayStation3;

class ThreadManager {
public:
    ThreadManager(PlayStation3* ps3) : ps3(ps3) {
        // Avoid reallocations
        // My code relies on this... I know it's bad. I pass pointers to Thread objects to the scheduler, if we reallocate
        // the pointers will become invalid
        // TODO: definitely find a better way to do this
        threads.reserve(256);   // More than enough
    }
    PlayStation3* ps3;

    Thread* createThread(u64 entry, u64 stack_size, u64 arg, s32 prio, const u8* name, u32 tls_vaddr, u32 tls_filesize, u32 tls_memsize, bool is_start_thread = false, bool is_emulator_thread = false, std::string executable_path = "");
    void contextSwitch(Thread& thread);
    Thread* getCurrentThread();
    Thread* getThreadByID(u32 id);
    void reschedule();
    void setAllHighPriority();
    std::vector<Thread> threads;
    u64 current_thread_id;
    u64 idle_thread_id;

    u64 allocateStack(u64 stack_size);

    void setTLS(u32 tls_vaddr, u32 tls_filesize, u32 tls_memsize);
    u32 allocTLS(u32 size);

    u32 tls_vaddr;
    u32 tls_filesize;
    u32 tls_memsize;

private:
    MAKE_LOG_FUNCTION(log, thread);
    u32 next_thread_id = 0x10000000;
    u32 next_emu_thread_id = 0x1000;
};
