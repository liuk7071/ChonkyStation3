#include "Lv2Semaphore.hpp"
#include "PlayStation3.hpp"


void Lv2Semaphore::post(u32 val) {
    this->val += val;

    while (wait_list.size()) {
        if (this->val == 0) break;
        // Wake up thread
        Thread* t = ps3->thread_manager.getThreadByID(wait_list.front());
        t->wakeUp();
        wait_list.pop_front();
        // Decrement semaphore
        this->val--;
        // Cancel timeout events
        ps3->scheduler.deleteAllEventsOfName(std::format("timeout {:d}", t->id));
    }
}

void Lv2Semaphore::wait(u64 timeout) {
    if (val > 0)
        val--;
    else {
        auto curr_thread = ps3->thread_manager.getCurrentThread();
        curr_thread->wait(std::format("sema {}", handle()));
        if (timeout) curr_thread->timeout(timeout);
        wait_list.push_back(ps3->thread_manager.getCurrentThread()->id);
    }
}
