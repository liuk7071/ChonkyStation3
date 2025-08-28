#include <Syscall.hpp>
#include "PlayStation3.hpp"


MAKE_LOG_FUNCTION(log_sys_timer, sys_timer);

u64 Syscall::sys_timer_usleep() {
    const u64 us = ARG0;
    log_sys_timer("sys_timer_usleep(us: %d) @ 0x%08x\n", us, (u32)ps3->ppu->state.pc);

    ps3->thread_manager.getCurrentThread()->sleep(us);

    return CELL_OK;
}

u64 Syscall::sys_timer_sleep() {
    const u64 s = ARG0;
    log_sys_timer("sys_timer_sleep(s: %d)\n", s);

    ps3->thread_manager.getCurrentThread()->sleep(s * 1000000);
    //ps3->thread_manager.getCurrentThread()->sleep(s * 1);

    return CELL_OK;
}
