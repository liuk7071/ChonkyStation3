#pragma once

#include <common.hpp>
#include <BEField.hpp>

#include <queue>

#include <Lv2Base.hpp>


class Lv2EventQueue : public virtual Lv2Base {
public:
    Lv2EventQueue(Lv2Object* obj) : Lv2Base(obj) {}

    struct Event {
        u64 source;
        u64 data1;
        u64 data2;
        u64 data3;
    };
    
    // Big endian CellOS equivalent struct (we could also just use this struct only for everything)
    struct sys_event {
        BEField<u64> source;
        BEField<u64> data1;
        BEField<u64> data2;
        BEField<u64> data3;
    };

    u32 size = 0;
    std::queue<u64> wait_list;
    std::queue<Event> events;
    bool is_connected_to_spu_event_port = false;    // For debugging
    
    void receive(u64 timeout);
    s32  tryreceive(u32 event_array_ptr, s32 size);
    void send(Event event);
    void drain();
};
