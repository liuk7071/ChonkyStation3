#pragma once

#include <common.hpp>

#include <unordered_map>

#include <SPUThread.hpp>


// Circular dependency
class PlayStation3;

class SPUThreadManager {
public:
    SPUThreadManager(PlayStation3* ps3) : ps3(ps3) {
        threads.reserve(128);   // Avoid reallocations
    }
    PlayStation3* ps3;

    SPUThread* createThread(std::string name, bool is_raw = false, int raw_idx = -1);
    void contextSwitch(SPUThread& thread);
    SPUThread* getCurrentThread();
    SPUThread* getThreadByID(u32 id);
    void reschedule();

    std::vector<SPUThread> threads;
    u64 current_thread_id = 0;
    
    // Reservations
    struct Reservation {
        u32 addr;
        u8 data[128];
    };
    void createReservation(u32 addr);
    bool acquireReservation(u32 addr);
    void reservationWritten(u64 vaddr);
    std::unordered_map<u32, Reservation> reservation_map;   // first is thread id, 2nd is address

private:
    MAKE_LOG_FUNCTION(log, thread_spu);
};
