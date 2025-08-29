#include "Thread.hpp"
#include "ThreadManager.hpp"
#include "PlayStation3.hpp"


Thread::Thread(u64 entry, u64 stack_size, u64 arg, s32 prio, const u8* name, u32 id, u32 tls_vaddr, u32 tls_filesize, u32 tls_memsize, ThreadManager* mgr) : mgr(mgr) {
    const u32 real_entry = mgr->ps3->mem.read<u32>(entry);
    this->id = id;
    this->name = Helpers::readString(name);
    this->prio = prio;

    // TODO: stack size should also be aligned to 0x1000 byte boundary
    if (stack_size < 0x1000) stack_size = 0x1000;

    stack = mgr->allocateStack(stack_size);
    this->stack_size = stack_size;

    const u64 sp = stack + stack_size;
    state.pc = real_entry;
    
    state.gprs[1] = sp - 8;
    state.gprs[2] = mgr->ps3->mem.read<u32>(entry + 4);

    if (tls_vaddr) {
        // Initialize TLS
        mgr->ps3->module_manager.sysThread.initializeTLS(id, tls_vaddr, tls_filesize, tls_memsize, state);
    }

    state.gprs[3] = arg;
    
    func_done_flag_queue.reserve(128);
}

void Thread::addArg(u64 arg) {
    args.push_back(arg);
}

void Thread::finalizeArgs() {
    for (int i = 0; i < args.size(); i++) {
        state.gprs[1] -= 8;
        mgr->ps3->mem.write<u64>(state.gprs[1], args[i]);
    }

    state.gprs[3] = args.size();
    state.gprs[4] = state.gprs[1];
    printf("argc for thread \"%s\": %lld\n", name.c_str(), state.gprs[3]);
    printf("argv for thread \"%s\": 0x%08x\n", name.c_str(), (u32)state.gprs[4]);

    state.gprs[28] = state.gprs[4];
    state.gprs[29] = state.gprs[3];
}

void Thread::addEnv(u64 arg) {
    env.push_back(arg);
}

void Thread::finalizeEnv() {
    for (int i = 0; i < env.size(); i++) {
        state.gprs[1] -= 8;
        mgr->ps3->mem.write<u64>(state.gprs[1], env[i]);
    }

    state.gprs[6] = env.size();
    state.gprs[5] = state.gprs[1];
    printf("envc for thread \"%s\": %lld\n", name.c_str(), state.gprs[6]);
    printf("envv for thread \"%s\": 0x%08x\n", name.c_str(), (u32)state.gprs[5]);

    state.gprs[31] = state.gprs[5];
}

void Thread::finalizeArgsAndEnv() {
    state.gprs[1] -= 8;
    if (state.gprs[1] & 0xf)
        state.gprs[1] -= 8;

    // Should be unreachable.
    // The args/env variables we push are all u64s, if we are still not aligned to 16 byte boundary after subtracting 8 it means something went wrong.
    if (state.gprs[1] & 0xf)
        Helpers::panic("Bad stack alignment after arg and env setup: 0x%08x\n", state.gprs[1]);

    state.gprs[1] -= 0x70;
    //printf("sp for thread \"%s\": 0x%08x\n", name.c_str(), (u32)state.gprs[1]);
}

// We put the reschedule on the scheduler instead of having it happen instantly because it would break
// if a reschedule is called in the middle of an HLE function
void Thread::reschedule(u64 cycles) {
    mgr->ps3->scheduler.push(std::bind(&ThreadManager::reschedule, mgr), cycles, "thread reschedule");
}

void Thread::sleep(u64 us) {
    const u64 cycles = Scheduler::uSecondsToCycles(us);
    mgr->ps3->scheduler.push(std::bind(&Thread::wakeUp, this), cycles, "thread wakeup");
    status = ThreadStatus::Sleeping;
    reschedule();
    mgr->setAllHighPriority();
    log("Sleeping thread %d for %d us\n", id, us);
}

void Thread::sleepForCycles(u64 cycles) {
    mgr->ps3->scheduler.push(std::bind(&Thread::wakeUp, this), cycles, "thread wakeup");
    status = ThreadStatus::Sleeping;
    reschedule();
    log("Sleeping thread %d for %lld cycles\n", id, cycles);
}

void Thread::wait(std::string wait_reason) {
    status = ThreadStatus::Waiting;
    this->wait_reason = wait_reason;
    reschedule();
    log("Thread %d \"%s\" is waiting\n", id, name.c_str());
}

void Thread::timeout(u64 us) {
    const u64 cycles = Scheduler::uSecondsToCycles(us);
    mgr->ps3->scheduler.push(std::bind(&Thread::timeoutEvent, this), cycles, std::format("timeout {:d}", id));
    reschedule();
}

void Thread::timeoutEvent() {
    state.gprs[3] = CELL_ETIMEDOUT;
    log("Timed out, waking up\n");
    wakeUp();
}

void Thread::wakeUp() {
    status = ThreadStatus::Running;
    wait_reason = "";
    reschedule();
    log("Woke up thread %d \"%s\"\n", id, name.c_str());
}

void Thread::join(u32 id, u32 vptr) {
    waiter = id;
    this->vptr = vptr;
    mgr->getThreadByID(waiter)->wait(std::format("joining thread {:s}", name));
}

void Thread::exit(u64 exit_status) {
    status = ThreadStatus::Terminated;
    reschedule();
    log("Thread %d \"%s\" exited with status %d\n", id, name.c_str(), exit_status);
    this->exit_status = exit_status;

    // Wake up thread waiting to join this thread
    if (waiter) {
        mgr->getThreadByID(waiter)->wakeUp();
        mgr->ps3->mem.write<u64>(vptr, exit_status);
    }
}
