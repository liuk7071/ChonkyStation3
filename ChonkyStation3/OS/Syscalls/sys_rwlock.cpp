#include <Syscall.hpp>
#include "PlayStation3.hpp"


MAKE_LOG_FUNCTION(log_sys_rwlock, sys_rwlock);

u64 Syscall::sys_rwlock_create() {
    const u32 rwlock_id_ptr = ARG0;
    const u32 attr_ptr = ARG1;
    log_sys_rwlock("sys_rwlock_create(rwlock_id_ptr: 0x%08x, attr_ptr: 0x%08x)\n", rwlock_id_ptr, attr_ptr);

    if (!attr_ptr)
        return CELL_EFAULT;

    Lv2RwLock* rwlock = ps3->lv2_obj.create<Lv2RwLock>();
    rwlock->initLocks();

    ps3->mem.write<u32>(rwlock_id_ptr, rwlock->handle());

    return CELL_OK;
}

u64 Syscall::sys_rwlock_rlock() {
    const u32 rwlock_id = ARG0;
    const u64 timeout = ARG1;
    log_sys_rwlock("sys_rwlock_rlock(rwlock_id: %d, timeout: 0x%lld)\n", rwlock_id, timeout);

    if (!ps3->lv2_obj.exists(rwlock_id))
        return CELL_ESRCH;

    Lv2RwLock* rwlock = ps3->lv2_obj.get<Lv2RwLock>(rwlock_id);
    rwlock->rlock();

    return CELL_OK;
}

u64 Syscall::sys_rwlock_runlock() {
    const u32 rwlock_id = ARG0;
    log_sys_rwlock("sys_rwlock_runlcok(rwlock_id: %d)\n", rwlock_id);

    if (!ps3->lv2_obj.exists(rwlock_id))
        return CELL_ESRCH;

    Lv2RwLock* rwlock = ps3->lv2_obj.get<Lv2RwLock>(rwlock_id);
    rwlock->runlock();

    return CELL_OK;
}

u64 Syscall::sys_rwlock_wlock() {
    const u32 rwlock_id = ARG0;
    const u64 timeout = ARG1;
    log_sys_rwlock("sys_rwlock_wlock(rwlock_id: %d, timeout: 0x%lld)\n", rwlock_id, timeout);

    if (!ps3->lv2_obj.exists(rwlock_id))
        return CELL_ESRCH;

    Lv2RwLock* rwlock = ps3->lv2_obj.get<Lv2RwLock>(rwlock_id);
    rwlock->wlock();

    return CELL_OK;
}

u64 Syscall::sys_rwlock_wunlock() {
    const u32 rwlock_id = ARG0;
    log_sys_rwlock("sys_rwlock_wunlcok(rwlock_id: %d)\n", rwlock_id);

    if (!ps3->lv2_obj.exists(rwlock_id))
        return CELL_ESRCH;

    Lv2RwLock* rwlock = ps3->lv2_obj.get<Lv2RwLock>(rwlock_id);
    rwlock->wunlock();

    return CELL_OK;
}