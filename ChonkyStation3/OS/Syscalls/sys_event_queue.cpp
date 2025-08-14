#include <Syscall.hpp>
#include "PlayStation3.hpp"
#include <Lv2Objects/Lv2EventQueue.hpp>


MAKE_LOG_FUNCTION(log_sys_event, sys_event);

u64 Syscall::sys_event_queue_create() {
    const u32 queue_id_ptr = ARG0;
    const u32 attr_ptr = ARG1;
    const u64 ipc_key = ARG2;
    const s32 size = ARG3;
    log_sys_event("sys_event_queue_create(queue_id_ptr: 0x%08x, attr_ptr: 0x%08x, ipc_key: 0x%016x, size: 0x%08x)\n", queue_id_ptr, attr_ptr, ipc_key, size);

    Lv2EventQueue* queue = ps3->lv2_obj.create<Lv2EventQueue>();
    queue->size = size;

    ps3->mem.write<u32>(queue_id_ptr, queue->handle());
    return CELL_OK;
}

u64 Syscall::sys_event_queue_receive() {
    const u32 queue_id = ARG0;
    const u32 event_ptr = ARG1; // Unused
    const u64 timeout = ARG2;
    log_sys_event("sys_event_queue_receive(queue_id: %d, event_ptr: 0x%08x, timeout: %d)\n", queue_id, event_ptr, timeout );

    Lv2EventQueue* queue = ps3->lv2_obj.get<Lv2EventQueue>(queue_id);
    queue->receive(timeout);    // queue->receive will return the event data in gprs[4] - gprs[7]

    return CELL_OK;
}

u64 Syscall::sys_event_queue_tryreceive() {
    const u32 queue_id = ARG0;
    const u32 event_array_ptr = ARG1;
    const s32 size = ARG2;      // Size of event array
    const u32 num_ptr = ARG3;   // Number of events received
    log_sys_event("sys_event_queue_tryreceive(queue_id: %d, event_array_ptr: 0x%08x, size: %d, num_ptr: 0x%08x)\n", queue_id, event_array_ptr, size, num_ptr);
    
    Lv2EventQueue* queue = ps3->lv2_obj.get<Lv2EventQueue>(queue_id);
    s32 num = queue->tryreceive(event_array_ptr, size);
    ps3->mem.write<u32>(num_ptr, num);
    
    return CELL_OK;
}

u64 Syscall::sys_event_queue_drain() {
    const u32 queue_id = ARG0;
    log_sys_event("sys_event_queue_drain(queue_id: %d)\n", queue_id);

    Lv2EventQueue* queue = ps3->lv2_obj.get<Lv2EventQueue>(queue_id);
    queue->drain();

    return CELL_OK;
}
