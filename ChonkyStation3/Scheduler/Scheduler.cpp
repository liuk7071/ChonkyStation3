#include "Scheduler.hpp"


void Scheduler::tick(u64 cycles) {
    std::lock_guard<std::recursive_mutex> lock(mutex);
    time += cycles;

    while (events.size() && time >= events.top().time) {
        auto event = events.top();
        events.pop();
        event.func();
    }
}

// Returns whether or not there was a next event
bool Scheduler::tickToNextEvent(u64& elapsed) {
    std::lock_guard<std::recursive_mutex> lock(mutex);
    if (events.empty()) return false;
    
    elapsed = events.top().time - time;
    time = events.top().time;
    auto event = events.top();
    events.pop();

    //printf("Skipped to event \"%s\"\n", event.name.c_str());
    event.func();
    return true;
}

void Scheduler::push(std::function<void(void)> func, u64 time, std::string name) {
    std::lock_guard<std::recursive_mutex> lock(mutex);
    Helpers::debugAssert(events.size() < schedulerMaxEntries, "Scheduler: queued more than %d scheduler events\n", schedulerMaxEntries);

    Event event = { func, this->time + time, next_event_id++, name };
    events.push(event);
}

void Scheduler::deleteAllEventsOfName(std::string name) {
    std::lock_guard<std::recursive_mutex> lock(mutex);
    auto eventList = events.get_container();

    for (int i = 0; i < events.size(); i++) {
        for (auto& i : eventList) {
            if (i.name == name) events.remove(i);
        }
    }
}

u64 Scheduler::uSecondsToCycles(double us) {
    return CPU_FREQ * 0.000001 * us;
}
