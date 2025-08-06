#pragma once

#include <common.hpp>
#include <logger.hpp>

#include <CellTypes.hpp>
#include <Lv2Objects/Lv2Mutex.hpp>
#include <Lv2Objects/Lv2Cond.hpp>
#include <Lv2Objects/Lv2Semaphore.hpp>
#include <Lv2Objects/Lv2RwLock.hpp>


// Circular dependency
class PlayStation3;

using namespace CellTypes;

class Syscall {
public:
    Syscall(PlayStation3* ps3);
    PlayStation3* ps3;
    MAKE_LOG_FUNCTION(log_misc, misc_sc);
    MAKE_LOG_FUNCTION(unimpl, unimplemented);
    MAKE_LOG_FUNCTION(tty, tty);

    void doSyscall(bool decrement_pc_if_module_call = false);

    void todo(std::string name);
    // sys_mmapper
    u64 sys_mmapper_allocate_address();
    u64 sys_mmapper_free_shared_memory();
    u64 sys_mmapper_allocate_shared_memory();
    u64 sys_mmapper_map_shared_memory();
    u64 sys_mmapper_unmap_shared_memory();
    u64 sys_mmapper_search_and_map();

    // sys_memory
    u64 sys_memory_container_create();
    u64 sys_memory_container_destroy();
    u64 sys_memory_allocate();
    u64 sys_memory_free();
    u64 sys_memory_allocate_from_container();
    u64 sys_memory_get_page_attribute();
    u64 sys_memory_get_user_memory_size();

    // sys_vm
    u64 sys_vm_memory_map();
    u64 sys_vm_append_memory();
    u64 sys_vm_touch();

    // sys_timer
    u64 sys_timer_usleep();
    u64 sys_timer_sleep();

    // sys_event
    u64 sys_event_queue_create();
    u64 sys_event_queue_receive();
    u64 sys_event_queue_tryreceive();
    u64 sys_event_queue_drain();
    u64 sys_event_port_create();
    u64 sys_event_port_connect_local();
    u64 sys_event_port_send();

    // sys_ppu_thread
    u64 sys_ppu_thread_yield();
    u64 sys_ppu_thread_join();
    u64 sys_ppu_thread_get_priority();
    u64 sys_ppu_thread_get_stack_information();

    // sys_event_flag
    u64 sys_event_flag_create();
    u64 sys_event_flag_wait();
    u64 sys_event_flag_set();
    u64 sys_event_flag_clear();

    // sys_semaphore
    u64 sys_semaphore_create();
    u64 sys_semaphore_wait();
    u64 sys_semaphore_trywait();
    u64 sys_semaphore_post();
    u64 sys_semaphore_get_value();

    // sys_mutex
    u64 sys_mutex_create();
    u64 sys_mutex_lock();
    u64 sys_mutex_trylock();
    u64 sys_mutex_unlock();

    // sys_cond
    u64 sys_cond_create();
    u64 sys_cond_wait();
    u64 sys_cond_signal();
    u64 sys_cond_signal_all();

    // sys_rwlock
    u64 sys_rwlock_create();
    u64 sys_rwlock_rlock();
    u64 sys_rwlock_runlock();
    u64 sys_rwlock_wlock();
    u64 sys_rwlock_wunlock();

    // sys_prx
    u64 sys_prx_get_module_list();
    u64 sys_prx_get_module_info();
    u64 sys_prx_get_module_id_by_name();

    // sys_spu
    u64 sys_spu_image_open();
    u64 sys_raw_spu_create();
    u64 sys_spu_thread_group_create();
    u64 sys_spu_thread_initialize();
    u64 sys_spu_thread_group_start();
    u64 sys_spu_thread_group_join();
    u64 sys_spu_thread_read_ls();
    u64 sys_spu_thread_group_connect_event();
    u64 sys_spu_thread_write_spu_mb();
    u64 sys_spu_thread_connect_event();
    u64 sys_spu_thread_group_connect_event_all_threads();
    u64 sys_spu_image_import();

    // sys_fs
    u64 sys_fs_test();
    u64 sys_fs_fcntl();

    // Temporary until I move the FS syscalls to their own file
    enum CELL_FS_S : u32 {
        CELL_FS_S_IFDIR = 0040000,	// Directory
        CELL_FS_S_IFREG = 0100000,	// Regular
        CELL_FS_S_IFLNK = 0120000,	// Symbolic link
        CELL_FS_S_IFWHT = 0160000,	// Unknown

        CELL_FS_S_IRUSR = 0000400,	// R for owner
        CELL_FS_S_IWUSR = 0000200,	// W for owner
        CELL_FS_S_IXUSR = 0000100,	// X for owner

        CELL_FS_S_IRGRP = 0000040,	// R for group
        CELL_FS_S_IWGRP = 0000020,	// W for group
        CELL_FS_S_IXGRP = 0000010,	// X for group

        CELL_FS_S_IROTH = 0000004,	// R for other
        CELL_FS_S_IWOTH = 0000002,	// W for other
        CELL_FS_S_IXOTH = 0000001,	// X for other
    };
};
