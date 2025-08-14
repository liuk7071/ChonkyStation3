#include "Lv2EventQueue.hpp"
#include "PlayStation3.hpp"


void Lv2EventQueue::receive(u64 timeout) {
    Thread* curr_thread = ps3->thread_manager.getCurrentThread();
    if (!events.empty()) {
        const auto& event = events.front();
        ps3->ppu->state.gprs[4] = event.source;
        ps3->ppu->state.gprs[5] = event.data1;
        ps3->ppu->state.gprs[6] = event.data2;
        ps3->ppu->state.gprs[7] = event.data3;
        events.pop();
    }
    else {
        // Wait for an event
        curr_thread->wait(std::format("equeue {:d} {:s}", handle(), is_connected_to_spu_event_port ? "(connected to a SPU thread)" : ""));
        if (timeout) curr_thread->timeout(timeout);
        wait_list.push(curr_thread->id);
    }
}

// Returns the number of events received
s32 Lv2EventQueue::tryreceive(u32 event_array_ptr, s32 size) {
    sys_event* event_array = (sys_event*)ps3->mem.getPtr(event_array_ptr);
    s32 received = 0;
    
    // Size is in elements and NOT bytes
    for (int i = 0; i < size; i++) {
        if (!events.empty()) {
            const auto& event = events.front();
            event_array[i].source   = event.source;
            event_array[i].data1    = event.data1;
            event_array[i].data2    = event.data2;
            event_array[i].data3    = event.data3;
            events.pop();
            received++;
        } else break;
    }
    
    return received;
}

void Lv2EventQueue::send(Event event) {
    if (wait_list.empty()) {
        events.push(event);
    }
    else {
        // TODO: is this the correct behavior (sending the event to all waiting threads)?
        while (!wait_list.empty()) {
            // Wake up thread
            Thread* t = ps3->thread_manager.getThreadByID(wait_list.front());
            t->wakeUp();
            wait_list.pop();

            // Return event info in gprs[4] - gprs[7]
            const auto curr_thread = ps3->thread_manager.getCurrentThread()->id;
            ps3->thread_manager.contextSwitch(*t);
            ps3->ppu->state.gprs[4] = event.source;
            ps3->ppu->state.gprs[5] = event.data1;
            ps3->ppu->state.gprs[6] = event.data2;
            ps3->ppu->state.gprs[7] = event.data3;
            ps3->thread_manager.contextSwitch(*ps3->thread_manager.getThreadByID(curr_thread));
            
            // Cancel timeout events
            ps3->scheduler.deleteAllEventsOfName(std::format("timeout {:d}", t->id));
        }
    }
}

void Lv2EventQueue::drain() {
    while (events.size()) events.pop();
}
