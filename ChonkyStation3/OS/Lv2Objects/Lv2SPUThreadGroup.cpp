#include "Lv2SPUThreadGroup.hpp"
#include "Lv2EventQueue.hpp"
#include "PlayStation3.hpp"


// Returns true if the group was started successfully, false otherwise
bool Lv2SPUThreadGroup::start() {
    if (started) return false;

    // TODO: Check if all the threads in this group are ready
    if (!ps3->settings.debug.disable_spu) {
        for (auto& i : threads) {
            auto thread = ps3->spu_thread_manager.getThreadByID(i.id);
            sys_spu_image* img = (sys_spu_image*)ps3->mem.getPtr(i.img_ptr);

            thread->reset();
            // Load SPU image
            thread->loadImage(img);
            // Setup arguments
            thread->state.gprs[3].dw[1] = i.arg0;
            thread->state.gprs[4].dw[1] = i.arg1;
            thread->state.gprs[5].dw[1] = i.arg2;
            thread->state.gprs[6].dw[1] = i.arg3;
            
            thread->status = SPUThread::ThreadStatus::Running;
            // Delay waking the thread up
            //thread->sleep(50000);
            //if (!thread->name.contains("0")
            //    && !thread->name.contains("3")
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

bool Lv2SPUThreadGroup::stop(u32 exit_status) {
    if (!started) return false;
    
    started = false;
    for (auto& i : threads) {
        auto thread = ps3->spu_thread_manager.getThreadByID(i.id);
        thread->status = SPUThread::ThreadStatus::Ready;
    }
    
    // Wake up joining thread
    if (waiter) {
        ps3->mem.write<u32>(cause_ptr, 1);  // SYS_SPU_THREAD_GROUP_JOIN_GROUP_EXIT
        ps3->mem.write<u32>(status_ptr, exit_status);
        ps3->thread_manager.getThreadByID(waiter)->wakeUp();
        waiter = 0;
    }
    
    return true;
}

bool Lv2SPUThreadGroup::join(u32 id, u32 cause_ptr, u32 status_ptr) {
    if (waiter) return false;
    
    waiter = id;
    this->cause_ptr = cause_ptr;
    this->status_ptr = status_ptr;
    ps3->thread_manager.getThreadByID(waiter)->wait(std::format("joining SPU thread group {:d}", handle()));
    return true;
}

std::string Lv2SPUThreadGroup::getName() {
    return Helpers::readString(ps3->mem.getPtr(attr->name_ptr));
}
