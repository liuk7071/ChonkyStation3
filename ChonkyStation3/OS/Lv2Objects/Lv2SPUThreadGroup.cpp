#include "Lv2SPUThreadGroup.hpp"
#include "Lv2EventQueue.hpp"
#include "PlayStation3.hpp"


// Returns true if the group was started successfully, false otherwise
bool Lv2SPUThreadGroup::start() {
    if (started) return false;

    // TODO: Check if all the threads in this group are ready
    if (!ps3->settings.debug.disable_spu) {
        for (auto& i : threads) {
            auto thread = ps3->spu_thread_manager.getThreadByID(i);
            thread->status = SPUThread::ThreadStatus::Running;
            // Delay waking the thread up
            //thread->sleep(50000);
            //if (!thread->name.contains("0")
            //    && !thread->name.contains("1")
            //    )
            //    thread->status = SPUThread::ThreadStatus::Waiting;   // Only allow 1 SPURS thread to run, for debugging
        }
    }

    ps3->spu_thread_manager.reschedule();
    started = true;

    // Send run event
    if (run_event_queue_id) {
        Lv2EventQueue* queue = (Lv2EventQueue*)ps3->lv2_obj.get<Lv2EventQueue>(run_event_queue_id);
        queue->send({ SYS_SPU_THREAD_GROUP_EVENT_RUN_KEY, handle(), 0, 0 });
    }
    return true;
}

std::string Lv2SPUThreadGroup::getName() {
    return Helpers::readString(ps3->mem.getPtr(attr->name_ptr));
}
