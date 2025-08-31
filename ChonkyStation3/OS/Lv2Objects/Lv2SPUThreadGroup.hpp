#pragma once

#include <common.hpp>

#include <queue>

#include <Lv2Base.hpp>
#include <OS/Syscalls/sys_spu.hpp>


using namespace sys_spu;

class Lv2SPUThreadGroup : public virtual Lv2Base {
public:
    Lv2SPUThreadGroup(Lv2Object* obj) : Lv2Base(obj) {}

    struct ThreadInfo {
        u32 id;
        u32 img_ptr;
        u64 arg0;
        u64 arg1;
        u64 arg2;
        u64 arg3;
    };
    
    sys_spu_thread_group_attribute* attr;
    std::vector<ThreadInfo> threads;
    bool started = false;
    u32 run_event_queue_id = 0;

    bool start();
    bool stop(u32 exit_status);
    bool join(u32 id, u32 cause_ptr, u32 status_ptr);
    std::string getName();
    
private:
    u32 waiter = 0; // ID of the PPU thread joining this thread group
    u32 cause_ptr = 0;
    u32 status_ptr = 0;
};
