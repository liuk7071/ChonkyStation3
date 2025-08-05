#include "sys_mutex.hpp"
#include <Syscall.hpp>
#include "PlayStation3.hpp"


using namespace sys_mutex;

MAKE_LOG_FUNCTION(log_sys_mutex, sys_mutex);

u64 Syscall::sys_mutex_create() {
    const u32 mutex_id_ptr = ARG0;
    const u32 attr_ptr = ARG1;
    log_sys_mutex("sys_mutex_create(mutex_id_ptr: 0x%08x, attr_ptr: 0x%08x)\n", mutex_id_ptr, attr_ptr);

    sys_mutex_attribute* attr = (sys_mutex_attribute*)ps3->mem.getPtr(attr_ptr);
    Lv2Mutex* mutex = ps3->lv2_obj.create<Lv2Mutex>();
    if (attr->recursive == SYS_SYNC_RECURSIVE) mutex->recursive = true;
    else if (attr->recursive == SYS_SYNC_NOT_RECURSIVE) mutex->recursive = false;
    else Helpers::panic("sys_mutex_create: invalid attr->recursive\n");
    
    ps3->mem.write<u32>(mutex_id_ptr, mutex->handle());
    return CELL_OK;
}

u64 Syscall::sys_mutex_lock() {
    const u32 mutex_id = ARG0;
    const u64 timeout = ARG1;
    log_sys_mutex("sys_mutex_lock(mutex_idx: %d, timeout: %d)\n", mutex_id, timeout);

    if (!mutex_id) {
        log_sys_mutex("WARNING: sys_mutex_lock with mutex_id == 0\n");
        return CELL_ESRCH;
    }

    Lv2Mutex* mutex = ps3->lv2_obj.get<Lv2Mutex>(mutex_id);
    if (!mutex->lock()) {
        log_sys_mutex("WARNING: attempt to lock non-recursive mutex twice\n");  // The Guided Fate Paradox does this (and probably many others)
        return CELL_EDEADLK;
    }

    return CELL_OK;
}

u64 Syscall::sys_mutex_trylock() {
    const u32 mutex_id = ARG0;
    log_sys_mutex("sys_mutex_trylock(mutex_idx: %d)\n", mutex_id);

    if (!mutex_id) {
        log_sys_mutex("WARNING: sys_mutex_trylock with mutex_id == 0\n");
        return CELL_ESRCH;
    }

    Lv2Mutex* mutex = ps3->lv2_obj.get<Lv2Mutex>(mutex_id);
    if (!mutex->tryLock()) return CELL_EBUSY;

    return CELL_OK;
}

u64 Syscall::sys_mutex_unlock() {
    const u32 mutex_id = ARG0;
    log_sys_mutex("sys_mutex_unlock(mutex_id: %d)\n", mutex_id);

    if (!mutex_id) {
        log_sys_mutex("WARNING: sys_mutex_lock with mutex_id == 0\n");
        return CELL_ESRCH;
    }

    Lv2Mutex* mutex = ps3->lv2_obj.get<Lv2Mutex>(mutex_id);
    mutex->unlock();

    return CELL_OK;
}
