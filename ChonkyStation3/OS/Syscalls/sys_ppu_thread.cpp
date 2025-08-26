#include <Syscall.hpp>
#include "PlayStation3.hpp"


MAKE_LOG_FUNCTION(log_sys_ppu_thread, sys_ppu_thread);

struct sys_ppu_thread_stack_t {
    BEField<u32> addr;
    BEField<u32> size;
};

u64 Syscall::sys_ppu_thread_yield() {
    log_sys_ppu_thread("sys_ppu_thread_yield() @ 0x%08x\n", ps3->ppu->state.lr);
    
    ps3->thread_manager.getCurrentThread()->reschedule();
    return CELL_OK;
}

u64 Syscall::sys_ppu_thread_join() {
    const u32 thread_id = ARG0;
    const u32 vptr = ARG1;  // v is u64
    log_sys_ppu_thread("sys_ppu_thread_join(thread_id: %d, vptr: 0x%08x)\n", thread_id, vptr);

    if (ps3->thread_manager.getThreadByID(thread_id)->waiter)   // Was a thread already for termination of this thread?
        return CELL_EINVAL;

    if (ps3->thread_manager.getThreadByID(thread_id)->status != Thread::ThreadStatus::Terminated) {
        ps3->thread_manager.getThreadByID(thread_id)->join(ps3->thread_manager.getCurrentThread()->id, vptr);
    }
    else {
        ps3->mem.write<u64>(vptr, ps3->thread_manager.getThreadByID(thread_id)->exit_status);
    }

    return CELL_OK;
}

u64 Syscall::sys_ppu_thread_get_priority() {
    const u32 thread_id = ARG0;
    const u32 prio_ptr = ARG1;
    log_sys_ppu_thread("sys_ppu_thread_get_priority(thread_id: %d, prio_ptr: 0x%08x)\n");

    ps3->mem.write<u32>(prio_ptr, ps3->thread_manager.getThreadByID(thread_id)->prio);
    return CELL_OK;
}

u64 Syscall::sys_ppu_thread_get_stack_information() {
    const u32 info_ptr = ARG0;
    log_sys_ppu_thread("sys_ppu_thread_get_stack_information(info_ptr: 0x%08x)\n", info_ptr);

    sys_ppu_thread_stack_t* stack = (sys_ppu_thread_stack_t*)ps3->mem.getPtr(info_ptr);
    stack->addr = ps3->thread_manager.getCurrentThread()->stack;
    stack->size = ps3->thread_manager.getCurrentThread()->stack_size;

    return CELL_OK;
}
