#include <Syscall.hpp>
#include "PlayStation3.hpp"
#include <SPU/SPULoader.hpp>
#include <Lv2Objects/Lv2SPUThreadGroup.hpp>
#include <Lv2Objects/Lv2EventQueue.hpp>
#include "sys_spu.hpp"


MAKE_LOG_FUNCTION(log_sys_spu, sys_spu);

using namespace sys_spu;

u64 Syscall::sys_spu_image_open() {
    const u32 image_ptr = ARG0;
    const u32 path_ptr = ARG1;
    const std::string path = Helpers::readString(ps3->mem.getPtr(path_ptr));
    log_sys_spu("sys_spu_image_open(image_ptr: 0x%08x, path_ptr: 0x%08x) [path: %s]\n", image_ptr, path_ptr, path.c_str());

    SPULoader loader = SPULoader(ps3);
    sys_spu_image* image = (sys_spu_image*)ps3->mem.getPtr(image_ptr);
    loader.load(ps3->fs.guestPathToHost(path), image);

    return CELL_OK;
}

u64 Syscall::sys_raw_spu_create() {
    const u32 id_ptr = ARG0;
    const u32 attr_ptr = ARG1;
    log_sys_spu("sys_raw_spu_create(id_ptr: 0x%08x, attr_ptr: 0x%08x)\n", id_ptr, attr_ptr);

    // Find an available raw SPU index
    int idx;
    for (idx = 0; idx < 5; idx++) {
        if (!raw_spu_ids[idx]) break;
    }
    if (idx == 5)
        Helpers::panic("Tried to create a raw SPU thread, but no raw SPU index was available\n");
    
    // Create the thread
    auto thread = ps3->spu_thread_manager.createThread(std::format("Raw SPU #{:d}", idx), true, idx);
    raw_spu_ids[idx] = thread->id;

    ps3->mem.write<u32>(id_ptr, idx);
    return CELL_OK;
}

u64 Syscall::sys_spu_thread_group_create() {
    const u32 id_ptr = ARG0;
    const u32 num = ARG1;
    const u32 prio = ARG2;
    const u32 attr_ptr = ARG3;
    log_sys_spu("sys_spu_thread_group_create(id_ptr: 0x%08x, num: %d, prio: %d, attr_ptr: 0x%08x)\n", id_ptr, num, prio, attr_ptr);

    Lv2SPUThreadGroup* group = ps3->lv2_obj.create<Lv2SPUThreadGroup>();
    sys_spu_thread_group_attribute* attr = (sys_spu_thread_group_attribute*)ps3->mem.getPtr(attr_ptr);
    group->attr = attr;

    log_sys_spu("Created SPU thread group \"%s\"\n", group->getName().c_str());
    ps3->mem.write<u32>(id_ptr, group->handle());

    return CELL_OK;
}

u64 Syscall::sys_spu_thread_initialize() {
    const u32 id_ptr = ARG0;
    const u32 group_id = ARG1;
    const u32 spu_num = ARG2;
    const u32 img_ptr = ARG3;
    const u32 attr_ptr = ARG4;
    const u32 arg_ptr = ARG5;
    log_sys_spu("sys_spu_thread_initialize(id_ptr: 0x%08x, group_id: %d, spu_num: %d, img_ptr: 0x%08x, attr_ptr: 0x%08x, arg_ptr: 0x%08x)\n", id_ptr, group_id, spu_num, img_ptr, attr_ptr, arg_ptr);

    sys_spu_image* img = (sys_spu_image*)ps3->mem.getPtr(img_ptr);
    sys_spu_thread_attribute* attr = (sys_spu_thread_attribute*)ps3->mem.getPtr(attr_ptr);
    sys_spu_thread_argument* arg = (sys_spu_thread_argument*)ps3->mem.getPtr(arg_ptr);
    const auto name = attr->name_ptr ? Helpers::readString(ps3->mem.getPtr(attr->name_ptr)) : "UNNAMED";
    
    // Create thread and load image
    auto thread = ps3->spu_thread_manager.createThread(name);

    // Register this thread in the SPU thread group
    Lv2SPUThreadGroup* group = ps3->lv2_obj.get<Lv2SPUThreadGroup>(group_id);
    const u32 id = ((group->threads.size() & 0xff) << 24) | (group_id & 0xffffff);
    thread->setID(id);
    thread->setGroupID(group_id);
    group->threads.push_back({ thread->id, img_ptr, arg->arg1, arg->arg2, arg->arg3, arg->arg4 });

    ps3->mem.write<u32>(id_ptr, id);

    return CELL_OK;
}

u64 Syscall::sys_spu_thread_group_start() {
    const u32 group_id = ARG0;
    log_sys_spu("sys_spu_thread_group_start(group_id: %d)\n", group_id);

    Lv2SPUThreadGroup* group = ps3->lv2_obj.get<Lv2SPUThreadGroup>(group_id);
    group->start();

    return CELL_OK;
}

u64 Syscall::sys_spu_image_import() {
    const u32 image_ptr = ARG0;
    const u32 src = ARG1;
    const u32 size = ARG2;
    const u32 arg4 = ARG3;
    log_sys_spu("sys_spu_image_import(image_ptr: 0x%08x, src: 0x%08x, size: 0x%08x, arg4: 0x%08x) UNIMPLEMENTED\n", image_ptr, src, size);

    Helpers::panic("TODO: sys_spu_image_import syscall\n");
    return CELL_OK;
}

u64 Syscall::sys_spu_thread_group_join() {
    const u32 group_id = ARG0;
    const u32 cause_ptr = ARG1;
    const u32 status_ptr = ARG2;
    log_sys_spu("sys_spu_thread_group_join(group_id: %d, cause_ptr: 0x%08x, status_ptr: 0x%08x)\n", group_id, cause_ptr, status_ptr);

    Lv2SPUThreadGroup* group = ps3->lv2_obj.get<Lv2SPUThreadGroup>(group_id);
    if (!group->started) {
        Helpers::panic("TODO: sys_spu_thread_group_join but the thread group already terminated\n");
    }
    
    // Join the thread group
    if (!group->join(ps3->thread_manager.getCurrentThread()->id, cause_ptr, status_ptr)) {
        Helpers::panic("TODO: sys_spu_thread_group_join but there already was a waiting thread\n");
    }

    return CELL_OK;
}

u64 Syscall::sys_spu_thread_read_ls() {
    const u32 id = ARG0;
    const u32 ls_addr = ARG1;
    const u32 val_ptr = ARG2;   // val is u64
    const u32 type = ARG3;
    //log_sys_spu("sys_spu_thread_read_ls(id: %d, ls_addr: 0x%08x, val_ptr: 0x%08x, type: %d)\n", id, ls_addr, val_ptr, type);

    SPUThread* thread = ps3->spu_thread_manager.getThreadByID(id);
    Helpers::debugAssert(thread != nullptr,  "sys_spu_thread_read_ls: bad thread id (%d)\n", id);
    Helpers::debugAssert(ls_addr <= 0x3ffff, "sys_spu_thread_read_ls: ls_addr is out of bounds (0x%08x)\n", ls_addr);

    switch (type) {
    case 1:     ps3->mem.write<u64>(val_ptr, thread->ls[ls_addr]);                                   break;
    case 2:     ps3->mem.write<u64>(val_ptr, Helpers::bswap<u16>(*(u16*)&thread->ls[ls_addr]));      break;
    case 4:     ps3->mem.write<u64>(val_ptr, Helpers::bswap<u32>(*(u32*)&thread->ls[ls_addr]));      break;
    case 8:     ps3->mem.write<u64>(val_ptr, Helpers::bswap<u64>(*(u64*)&thread->ls[ls_addr]));      break;
    default:    Helpers::panic("sys_spu_thread_read_ls: invalid type %d\n", type);
    }

    return CELL_OK;
}

u64 Syscall::sys_spu_thread_group_connect_event() {
    const u32 group_id = ARG0;
    const u32 queue_id = ARG1;
    const u32 event_type = ARG2;
    log_sys_spu("sys_spu_thread_group_connect_event(group_id: %d, queue_id: %d, event_type: 0x%08x)\n", group_id, queue_id, event_type);

    Lv2SPUThreadGroup* group = ps3->lv2_obj.get<Lv2SPUThreadGroup>(group_id);
    if (group->run_event_queue_id)
        Helpers::panic("Thread group already had a connected event queue\n");

    switch (event_type) {

    case SYS_SPU_THREAD_GROUP_EVENT_RUN:            group->run_event_queue_id = queue_id;   break;
    case SYS_SPU_THREAD_GROUP_EVENT_EXCEPTION:      /* TODO */                              break;
    case SYS_SPU_THREAD_GROUP_EVENT_SYSTEM_MODULE:  /* TODO */                              break;

    default:
        Helpers::panic("sys_spu_thread_group_connect_event: Unimplemented event type 0x%08x\n", event_type);
    }

    return CELL_OK;
}

u64 Syscall::sys_spu_thread_write_spu_mb() {
    const u32 id = ARG0;
    const u32 val = ARG1;
    log_sys_spu("sys_spu_thread_write_spu_mb(id: %d, val: 0x%08x)\n", id, val);

    SPUThread* thread = ps3->spu_thread_manager.getThreadByID(id);
    Helpers::debugAssert(thread != nullptr, "sys_spu_thread_write_spu_mb: bad thread id (%d)\n", id);
    thread->writeInMbox(val);

    return CELL_OK;
}

u64 Syscall::sys_spu_thread_connect_event() {
    const u32 id = ARG0;
    const u32 equeue_id = ARG1;
    const u32 etype = ARG2;
    const u32 spup = ARG3;
    log_sys_spu("sys_spu_thread_connect_event(id: %d, equeue_id: %d, etype: 0x%08x, spup: %d)\n", id, equeue_id, etype, spup);
    Helpers::debugAssert(etype == SYS_SPU_THREAD_EVENT_USER, "sys_spu_thread_connect_event: etype != SYS_SPU_THREAD_EVENT_USER\n");

    SPUThread* thread = ps3->spu_thread_manager.getThreadByID(id);
    Helpers::debugAssert(thread != nullptr, "sys_spu_thread_connect_event: bad thread id (%d)\n", id);

    if (thread->ports[spup] != -1) Helpers::panic("sys_spu_thread_connect_event: port was already connected\n");
    thread->ports[spup] = equeue_id;
    ps3->lv2_obj.get<Lv2EventQueue>(equeue_id)->is_connected_to_spu_event_port = true;
    return CELL_OK;
}

u64 Syscall::sys_spu_thread_group_connect_event_all_threads() {
    const u32 id = ARG0;
    const u32 equeue_id = ARG1;
    const u64 req = ARG2;
    const u32 spup_ptr = ARG3;  // spup is u8
    log_sys_spu("sys_spu_thread_group_connect_event_all_threads(id: %d, equeue: %d, req: 0x%016x, spup_ptr: 0x%08x)\n", id, equeue_id, req, spup_ptr);

    Lv2SPUThreadGroup* group = ps3->lv2_obj.get<Lv2SPUThreadGroup>(id);
    // Find a port candidate available in all threads part of the group
    u8 spup;
    for (spup = 0; spup < 64; spup++) {
        if ((req >> spup) & 1) {
            // Check if this port is free in all threads
            bool port_ok = true;
            for (auto& thread : group->threads) {
                if (ps3->spu_thread_manager.getThreadByID(thread.id)->ports[spup] != -1) {
                    port_ok = false;
                    break;
                }
            }
            if (port_ok) break;
        }
    }
    
    if (spup == 64) {   // Bodycount relies on this
        log_sys_spu("sys_spu_thread_group_connect_event_all_threads: no available port candidate\n");
        return CELL_EISCONN;
    }
    
    for (auto& thread : group->threads) {
        ps3->spu_thread_manager.getThreadByID(thread.id)->ports[spup] = equeue_id;
    }
    
    ps3->lv2_obj.get<Lv2EventQueue>(equeue_id)->is_connected_to_spu_event_port = true;
    
    ps3->mem.write<u8>(spup_ptr, spup);
    return CELL_OK;
}
