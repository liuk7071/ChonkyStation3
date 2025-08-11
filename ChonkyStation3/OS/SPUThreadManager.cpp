#include "SPUThreadManager.hpp"
#include "PlayStation3.hpp"


SPUThread* SPUThreadManager::createThread(std::string name, bool is_raw, int raw_idx) {
    threads.push_back({ ps3, name, is_raw, raw_idx });
    threads.back().init();
    return &threads.back();
}

void SPUThreadManager::contextSwitch(SPUThread& thread) {
    ps3->spu->enabled = true;
    SPUThread* current_thread = getCurrentThread();
    if (current_thread && current_thread->id == thread.id) return;

    if (current_thread)
        log("Switched from thread %d \"%s\" to thread %d \"%s\"\n", current_thread->id, current_thread->name.c_str(), thread.id, thread.name.c_str());
    else
        log("Switched to thread \"%s\"\n", thread.name.c_str());

    if (current_thread)
        current_thread->state = ps3->spu->state;
    
    ps3->spu->state = thread.state;
    ps3->spu->ls = thread.ls;
    current_thread_id = thread.id;
}

SPUThread* SPUThreadManager::getCurrentThread() {
    return getThreadByID(current_thread_id);
}

SPUThread* SPUThreadManager::getThreadByID(u32 id) {
    SPUThread* thread = nullptr;
    for (auto& i : threads) {
        if (i.id == id) {
            thread = &i;
            break;
        }
    }
    return thread;
}

void SPUThreadManager::reschedule() {
    // No thread is currently active - find the first running thread and execute it
    if (current_thread_id == 0) {
        for (auto& i : threads) {
            if (i.status == SPUThread::ThreadStatus::Running) {
                contextSwitch(i);
                break;
            }
        }
        return;
    }

    // Find index of current thread
    int curr_thread = 0;
    while (threads[curr_thread].id != getCurrentThread()->id) curr_thread++;

    // Try to switch to the next running thread
    bool found_thread = false;
    for (int i = 0; i < threads.size(); i++) {
        SPUThread& t = threads[(curr_thread + 1 + i) % threads.size()];    // +1 because we want to begin searching from the thread after the current thread

        if (t.status == SPUThread::ThreadStatus::Running) {
            contextSwitch(t);
            found_thread = true;
            break;
        }
    }
    
    // No available threads, disable SPU
    if (!found_thread) {
        log("No thread available, disabling SPU (pc was 0x%08x)\n", ps3->spu->state.pc);
        // Save the current state
        getCurrentThread()->state = ps3->spu->state;
        ps3->spu->enabled = false;
        current_thread_id = 0;
    }
}

void SPUThreadManager::createReservation(u32 addr) {
    const auto id = current_thread_id;
    reservation_map[id].addr = addr;
    std::memcpy(reservation_map[id].data, ps3->mem.getPtr(addr), 128);
    
    // Setup memory watchpoints
    for (u32 curr_addr = addr; curr_addr < addr + 128; curr_addr++) {
        ps3->mem.watchpoints_w[curr_addr] = std::bind(&SPUThreadManager::reservationWritten, this, std::placeholders::_1);
        // Slowmem for writes (to trigger the watchpoints)
        // We do this in the loop because the SPU could technically reserve an area that crosses page boundaries (although very unlikely to happen)
        ps3->mem.markAsSlowMem(curr_addr >> PAGE_SHIFT, false, true);
    }
}

// Returns whether the reservation was acquired successfully
bool SPUThreadManager::acquireReservation(u32 addr) {
    const auto id = current_thread_id;
    if (reservation_map.contains(id)) {
        if (reservation_map[id].addr != addr) {
            reservation_map.erase(id);
            return false;
        } else {
            reservation_map.erase(id);
            
            // Lose the reservation for any other threads that reserved the same address
            std::vector<u32> woken_up;
            for (auto [other_id, reservation] : reservation_map) {
                if (addr == reservation.addr) {
                    ps3->scheduler.push(std::bind(&SPUThread::sendLocklineLostEvent, getThreadByID(other_id)), 5000);
                    //getThreadByID(other_id)->sendLocklineLostEvent();
                    woken_up.push_back(other_id);
                }
            }
            
            for (auto i : woken_up) {
                reservation_map.erase(i);
            }
            return true;
        }
    } else return false;
}

void SPUThreadManager::reservationWritten(u64 vaddr) {
    std::vector<u32> woken_up;
    for (auto [id, reservation] : reservation_map) {
        if (Helpers::inRangeSized<u32>(vaddr, reservation.addr, 128)) {
            // Check if the data changed
            if (std::memcmp(reservation.data, ps3->mem.getPtr(reservation.addr), 128)) {
                ps3->scheduler.push(std::bind(&SPUThread::sendLocklineLostEvent, getThreadByID(id)), 10000);
                //getThreadByID(id)->sendLocklineLostEvent();
                woken_up.push_back(id);
            }
        }
    }
    
    for (auto id : woken_up) {
        log("Lockline written, woke up thread %d\n", id);
        reservation_map.erase(id);
    }
}
