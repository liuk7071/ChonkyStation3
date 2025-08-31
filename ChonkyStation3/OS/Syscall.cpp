#include "Syscall.hpp"
#include <PlayStation3.hpp>


Syscall::Syscall(PlayStation3* ps3) {
    this->ps3 = ps3;
}

void Syscall::todo(std::string name) {
    unimpl((name + " UNIMPLEMENTED\n").c_str());
    ps3->ppu->state.gprs[3] = CELL_OK;;
}

void Syscall::doSyscall(bool decrement_pc_if_module_call) {
    const auto syscall_num = ps3->ppu->state.gprs[11];

    const u16 stub = ps3->mem.read<u32>(ps3->ppu->state.pc) & 0xffff;
    switch (stub) {
    // Module call
    case 0x10: {
        // Normally the stub does this to jump to the function (mtctr + bctr), I don't think it matters but to be sure I added this here
        ps3->ppu->state.ctr = ps3->ppu->state.gprs[0];
        // import addr is stored in r12
        ps3->module_manager.call(ps3->module_manager.imports[ps3->ppu->state.gprs[12]]);
        return;
    }
    case 0x2010: {
        // Normally the stub does this to jump to the function (mtctr + bctr), I don't think it matters but to be sure I added this here
        ps3->ppu->state.ctr = ps3->ppu->state.gprs[0];

        const u32 stub_addr = ps3->mem.read<u32>(ps3->ppu->state.gprs[2]);
        ps3->module_manager.call(ps3->module_manager.imports[stub_addr]);
        return;
    }
    case 0x1010: {
        ps3->module_manager.lle(ps3->module_manager.imports[ps3->ppu->state.gprs[12]]);
        return;
    }
    }

    switch (syscall_num) {
    
    // cellGcmCallback
    case 0x400: ps3->ppu->state.gprs[3] = ps3->module_manager.cellGcmSys.cellGcmCallback(); break;
            
    case 0x3000: {
        // Return from func without restoring all state
        auto curr_thread = ps3->thread_manager.getCurrentThread();
        auto old_state = curr_thread->state_queue.back();
        ps3->ppu->state.pc = old_state.pc;
        ps3->ppu->state.lr = old_state.lr;
        ps3->ppu->state.gprs[2] = old_state.gprs[2];
        curr_thread->state_queue.pop_back();
        curr_thread->func_done_flag_queue[curr_thread->nested_func_calls - 1] = true;
        curr_thread->nested_func_calls--;
        break;
    }
            
    case 0x3001: {
        // Return from func and restore all state
        auto curr_thread = ps3->thread_manager.getCurrentThread();
        auto old_state = curr_thread->state_queue.back();
        ps3->ppu->state = old_state;
        curr_thread->state_queue.pop_back();
        curr_thread->func_done_flag_queue[curr_thread->nested_func_calls - 1] = true;
        curr_thread->nested_func_calls--;
        break;
    }
            
    // HLE vblank event
    case 4096: {
        ps3->vblank();
        break;
    }

    case 1: {
        log_misc("sys_process_getpid()\n");
        ps3->ppu->state.gprs[3] = 0x1000500;
        break;
    }
    case 14: {
        const u32 addr = ARG0;
        const u64 flags = ARG1;
        log_misc("sys_process_is_spu_lock_line_reservation_address(addr: 0x%08x, flags: 0x%016llx)\n", addr, flags);

        u32 ret = CELL_OK;
        switch (addr >> 28) {
        case 0xd:
        case 0xf:
            ret = CELL_EPERM;
        }

        ps3->ppu->state.gprs[3] = ret;
        break;
    }
    case 25: {
        const u32 pid = ARG0;
        const u32 ver_ptr = ARG1;
        // TODO: Get this from the elf's PROC_PARAM
        unimpl("sys_process_get_sdk_version() STUBBED\n");
        ps3->mem.write<u32>(ver_ptr, 0x00360001);
        ps3->ppu->state.gprs[3] = CELL_OK;
        break;
    }
    case 30:    todo("sys_process_get_paramsfo()");                                 break;
    case 43:    ps3->ppu->state.gprs[3] = sys_ppu_thread_yield();                   break;
    case 44:    ps3->ppu->state.gprs[3] = sys_ppu_thread_join();                    break;
    case 47:    todo("sys_ppu_thread_set_priority()");                              break;
    case 48:    ps3->ppu->state.gprs[3] = sys_ppu_thread_get_priority();            break;
    case 49:    ps3->ppu->state.gprs[3] = sys_ppu_thread_get_stack_information();   break;
    case 70:    todo("sys_timer_create()");                                         break;
    case 73:    todo("sys_timer_start()");                                          break;
    case 75:    todo("sys_timer_connect_event_queue()");                            break;
    case 82:    ps3->ppu->state.gprs[3] = sys_event_flag_create();                  break;
    case 83:    todo("sys_event_flag_destroy()");                                   break;
    case 85:    ps3->ppu->state.gprs[3] = sys_event_flag_wait();                    break;
    case 87:    ps3->ppu->state.gprs[3] = sys_event_flag_set();                     break;
    case 90:    ps3->ppu->state.gprs[3] = sys_semaphore_create();                   break;
    case 91:    todo("sys_semaphore_destroy()");                                    break;
    case 92:    ps3->ppu->state.gprs[3] = sys_semaphore_wait();                     break;
    case 93:    ps3->ppu->state.gprs[3] = sys_semaphore_trywait();                  break;
    case 94:    ps3->ppu->state.gprs[3] = sys_semaphore_post();                     break;
    case 95:    todo("_sys_lwmutex_create()");                                      break;
    case 96:    todo("_sys_lwmutex_destroy()");                                     break;
    case 97:    todo("_sys_lwmutex_lock()");                                        break;
    case 98:    todo("_sys_lwmutex_unlock()");                                      break;
    case 100:   ps3->ppu->state.gprs[3] = sys_mutex_create();                       break;
    case 101:   todo("sys_mutex_destroy()");                                        break;
    case 102:   ps3->ppu->state.gprs[3] = sys_mutex_lock();                         break;
    case 103:   ps3->ppu->state.gprs[3] = sys_mutex_trylock();                      break;
    case 104:   ps3->ppu->state.gprs[3] = sys_mutex_unlock();                       break;
    case 105:   ps3->ppu->state.gprs[3] = sys_cond_create();                        break;
    case 106:   todo("sys_cond_destroy()");                                         break;
    case 107:   ps3->ppu->state.gprs[3] = sys_cond_wait();                          break;
    case 108:   ps3->ppu->state.gprs[3] = sys_cond_signal();                        break;
    case 109:   ps3->ppu->state.gprs[3] = sys_cond_signal_all();                    break;
    case 114:   ps3->ppu->state.gprs[3] = sys_semaphore_get_value();                break;
    case 118:   ps3->ppu->state.gprs[3] = sys_event_flag_clear();                   break;
    case 120:   ps3->ppu->state.gprs[3] = sys_rwlock_create();                      break;
    case 121:   todo("sys_rwlock_destroy()");                                       break;
    case 122:   ps3->ppu->state.gprs[3] = sys_rwlock_rlock();                       break;
    case 124:   ps3->ppu->state.gprs[3] = sys_rwlock_runlock();                     break;
    case 125:   ps3->ppu->state.gprs[3] = sys_rwlock_wlock();                       break;
    case 127:   ps3->ppu->state.gprs[3] = sys_rwlock_wunlock();                     break;
    case 128:   ps3->ppu->state.gprs[3] = sys_event_queue_create();                 break;
    case 129:   todo("sys_event_queue_destroy()");                                  break;
    case 130:   ps3->ppu->state.gprs[3] = sys_event_queue_receive();                break;
    case 131:   ps3->ppu->state.gprs[3] = sys_event_queue_tryreceive();             break;
    case 133:   ps3->ppu->state.gprs[3] = sys_event_queue_drain();                  break;
    case 134:   ps3->ppu->state.gprs[3] = sys_event_port_create();                  break;
    case 135:   todo("sys_event_port_destroy()");                                   break;
    case 136:   ps3->ppu->state.gprs[3] = sys_event_port_connect_local();           break;
    case 137:   todo("sys_event_port_disconnect()");                                break;
    case 138:   ps3->ppu->state.gprs[3] = sys_event_port_send();                    break;
    case 141:   ps3->ppu->state.gprs[3] = sys_timer_usleep();                       break;
    case 142:   ps3->ppu->state.gprs[3] = sys_timer_sleep();                        break;
    case 144: {
        const u32 timezone_ptr = ARG0;
        const u32 summertime_ptr = ARG1;
        log_misc("sys_time_get_timezone()\n");
        ps3->mem.write<u32>(timezone_ptr, 60);  // timezone (60 == UTC+1 I think)
        ps3->mem.write<u32>(summertime_ptr, 0);   // summertime
        ps3->ppu->state.gprs[3] = CELL_OK;
        break;
    }
    case 145: {
        const u32 s_ptr = ARG0;
        const u32 us_ptr = ARG1;
        log_misc("sys_time_get_current_time()\n");
        const auto time = std::chrono::system_clock::now().time_since_epoch();
        ps3->mem.write<u64>(s_ptr, std::chrono::floor<std::chrono::seconds>(time).count());
        ps3->mem.write<u64>(us_ptr, std::chrono::floor<std::chrono::nanoseconds>(time).count());
        ps3->ppu->state.gprs[3] = CELL_OK;
        break;
    }
    case 147: {
        log_misc("sys_time_get_timebase_frequency()\n");
        ps3->ppu->state.gprs[3] = 80000000ull;
        break;
    }
    case 156:   ps3->ppu->state.gprs[3] = sys_spu_image_open();                                 break;
    case 160:   ps3->ppu->state.gprs[3] = sys_raw_spu_create();                                 break;
    case 169:   todo("sys_spu_initialize()");   /* Doesn't do anything important */             break;
    case 170:   ps3->ppu->state.gprs[3] = sys_spu_thread_group_create();                        break;
    case 172:   ps3->ppu->state.gprs[3] = sys_spu_thread_initialize();                          break;
    case 173:   ps3->ppu->state.gprs[3] = sys_spu_thread_group_start();                         break;
    case 174:   todo("sys_spu_thread_group_suspend()");                                         break;
    case 178:   ps3->ppu->state.gprs[3] = sys_spu_thread_group_join();                          break;
    case 182:   ps3->ppu->state.gprs[3] = sys_spu_thread_read_ls();                             break;
    case 185:   ps3->ppu->state.gprs[3] = sys_spu_thread_group_connect_event();                 break;
    case 190:   ps3->ppu->state.gprs[3] = sys_spu_thread_write_spu_mb();                        break;
    case 191:   ps3->ppu->state.gprs[3] = sys_spu_thread_connect_event();                       break;
    case 193:   todo("sys_spu_thread_bind_queue()");                                            break;
    case 250:   todo("sys_spu_thread_group_set_cooperative_victims()");                         break;
    case 251:   ps3->ppu->state.gprs[3] = sys_spu_thread_group_connect_event_all_threads();     break;
    case 252:   todo("sys_spu_thread_group_disconnect_event_all_threads()");                    break;
    case 253:   todo("sys_spu_thread_group_syscall_253()");                                     break;
    case 254:   todo("sys_spu_thread_group_log()");                                             break;
    case 300:   ps3->ppu->state.gprs[3] = sys_vm_memory_map();                                  break;
    case 302:   ps3->ppu->state.gprs[3] = sys_vm_append_memory();                               break;
    case 306:   ps3->ppu->state.gprs[3] = sys_vm_touch();                                       break;
    case 308:   todo("sys_vm_invalidate()");                                                    break;
    case 310:   todo("sys_vm_sync()");                                                          break;
    case 312:   todo("sys_vm_get_statistics()");                                                break;
    case 324:   ps3->ppu->state.gprs[3] = sys_memory_container_create();                        break;  // Debug
    case 327:   todo("sys_mmapper_enable_page_fault_notification()");                           break;
    case 329:   ps3->ppu->state.gprs[3] = sys_mmapper_free_shared_memory();                     break;
    case 330:   ps3->ppu->state.gprs[3] = sys_mmapper_allocate_address();                       break;
    case 331:   todo("sys_mmapper_free_address()");                                             break;
    case 332:   ps3->ppu->state.gprs[3] = sys_mmapper_allocate_shared_memory();                 break;
    case 334:   ps3->ppu->state.gprs[3] = sys_mmapper_map_shared_memory();                      break;
    case 335:   ps3->ppu->state.gprs[3] = sys_mmapper_unmap_shared_memory();                    break;
    case 337:   ps3->ppu->state.gprs[3] = sys_mmapper_search_and_map();                         break;
    case 341:   ps3->ppu->state.gprs[3] = sys_memory_container_create();                        break;
    case 342:   ps3->ppu->state.gprs[3] = sys_memory_container_destroy();                       break;
    case 348:   ps3->ppu->state.gprs[3] = sys_memory_allocate();                                break;
    case 349:   ps3->ppu->state.gprs[3] = sys_memory_free();                                    break;
    case 350:   ps3->ppu->state.gprs[3] = sys_memory_allocate_from_container();                 break;
    case 351:   ps3->ppu->state.gprs[3] = sys_memory_get_page_attribute();                      break;
    case 352:   ps3->ppu->state.gprs[3] = sys_memory_get_user_memory_size();                    break;
    case 383:   todo("sys_game_get_temperature()");                                             break;
    case 403: {   // puts
        const u32 str_ptr = ARG1;
        const u64 len = ARG2;
        std::string str;
        u8* ptr = ps3->mem.getPtr(str_ptr);
        for (int i = 0; i < len; i++)
            str += *ptr++;
        tty("%s", str.c_str());
        ps3->ppu->state.gprs[3] = CELL_OK;
        break;
    }
    case 462:   ps3->ppu->state.gprs[3] = CELL_OK;                  break;  // Debug syscall unavailable on retail consoles (liblv2 tries to use this)
    case 480:   ps3->ppu->state.gprs[3] = sys_prx_load_module();    break;
    case 481:   ps3->ppu->state.gprs[3] = sys_prx_start_module();   break;
    case 482:   ps3->ppu->state.gprs[3] = sys_prx_stop_module();    break;
    case 483:   todo("sys_prx_unload_module()");                    break;
    case 484: {
        const u32 name_ptr = ARG0;
        const u32 opt_ptr = ARG1;
        std::string name = Helpers::readString(ps3->mem.getPtr(name_ptr));
        unimpl("sys_prx_register_module(name_ptr: 0x%08x, opt_ptr: 0x%08x) [name: %s] UNIMPLEMENTED\n", name_ptr, opt_ptr, name.c_str());

        ps3->ppu->state.gprs[3] = CELL_OK;
        break;
    }
    case 486:   ps3->ppu->state.gprs[3] = sys_prx_register_library();                   break;
    case 494:   ps3->ppu->state.gprs[3] = sys_prx_get_module_list();                    break;
    case 495:   ps3->ppu->state.gprs[3] = sys_prx_get_module_info();                    break;
    case 496:   ps3->ppu->state.gprs[3] = sys_prx_get_module_id_by_name();              break;
    case 630:   todo("sys_gpio_set()");                                                 break;
    case 609:   todo("sys_storage_get_device_info()");                                  break;  // root
    case 631:   todo("sys_gpio_get()");                                                 break;
    case 801:   ps3->ppu->state.gprs[3] = ps3->module_manager.cellFs.cellFsOpen();      break;
    case 802: {
        if (!ARG3) Helpers::panic("sys_fs_read: bytes_read_ptr is null\n");
        ps3->ppu->state.gprs[3] = ps3->module_manager.cellFs.cellFsRead();
        break;
    }
    case 804:   ps3->ppu->state.gprs[3] = ps3->module_manager.cellFs.cellFsClose();     break;
    case 805:   ps3->ppu->state.gprs[3] = ps3->module_manager.cellFs.cellFsOpendir();   break;
    case 806:   ps3->ppu->state.gprs[3] = ps3->module_manager.cellFs.cellFsReaddir();   break;
    case 807:   ps3->ppu->state.gprs[3] = ps3->module_manager.cellFs.cellFsClosedir();  break;
    case 808:   ps3->ppu->state.gprs[3] = ps3->module_manager.cellFs.cellFsStat();      break;
    case 809: {
        const u32 fd = ARG0;
        const u32 stat_ptr = ARG1;

        if (fd == 1) {  // stdout?
            CellFsStat* stat = (CellFsStat*)ps3->mem.getPtr(stat_ptr);
            stat->mode = CELL_FS_S_IRUSR | CELL_FS_S_IWUSR | CELL_FS_S_IXUSR |
                         CELL_FS_S_IRGRP | CELL_FS_S_IWGRP | CELL_FS_S_IXGRP |
                         CELL_FS_S_IROTH | CELL_FS_S_IWOTH | CELL_FS_S_IXOTH |
                         CELL_FS_S_IFREG;
            stat->uid = 0;
            stat->gid = 0;
            stat->atime = 0;
            stat->mtime = 0;
            stat->ctime = 0;
            stat->size = 0;
            stat->blksize = 4096;
            ps3->ppu->state.gprs[3] = CELL_BADF;
        }
        else {
            ps3->ppu->state.gprs[3] = ps3->module_manager.cellFs.cellFsFstat();
        }
        break;
    }
    case 800:   ps3->ppu->state.gprs[3] = sys_fs_test();                                break;
    case 811:   ps3->ppu->state.gprs[3] = ps3->module_manager.cellFs.cellFsMkdir();     break;
    case 817:   ps3->ppu->state.gprs[3] = sys_fs_fcntl();                               break;
    case 818:   ps3->ppu->state.gprs[3] = ps3->module_manager.cellFs.cellFsLseek();     break;
    case 821:   ps3->ppu->state.gprs[3] = sys_fs_fget_block_size();                     break;
    case 825:   todo("sys_fs_lsn_get_cda_size()");                                      break;
    case 827:   todo("sys_fs_lsn_lock()");                                              break;
    case 828:   todo("sys_fs_lsn_unlock()");                                            break;
    case 871:   todo("sys_ss_access_control_engine()");                                 break;  // Debug
    case 872:   todo("sys_ss_get_open_psid()");                                         break;
    case 988:   ps3->ppu->state.gprs[3] = CELL_OK;                                      break;  // Debug

    default:
        Helpers::panic("Unimplemented syscall number 0x%02x (%d) @ 0x%016llx\n", syscall_num, syscall_num, ps3->ppu->state.pc);
    }
}
