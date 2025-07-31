#pragma once

#include <common.hpp>
#include <logger.hpp>
#include <BEField.hpp>

#include <thread>
#include <mutex>
#include <atomic>

#include <CellTypes.hpp>


// Circular dependency
class PlayStation3;

using namespace CellTypes;

static constexpr u64 CELL_AUDIO_PORT_2CH = 2;
static constexpr u64 CELL_AUDIO_PORT_8CH = 8;

static constexpr u64 CELL_AUDIO_BLOCK_8  =  8;
static constexpr u64 CELL_AUDIO_BLOCK_16 = 16;
static constexpr u64 CELL_AUDIO_BLOCK_32 = 32;

static constexpr u32 CELL_AUDIO_STATUS_CLOSE = 0x1010;
static constexpr u32 CELL_AUDIO_STATUS_READY = 1;
static constexpr u32 CELL_AUDIO_STATUS_RUN   = 2;


class CellAudio {
public:
    CellAudio(PlayStation3* ps3) : ps3(ps3) {
        for (int i = 0; i < 8; i++) ports[i].id = i;
    }
    PlayStation3* ps3;

    struct CellAudioPortParam {
        BEField<u64> n_channels;
        BEField<u64> n_blocks;
        BEField<u64> attr;
        BEField<float> level;
    };

    struct CellAudioPortConfig {
        BEField<u32> read_idx_addr;
        BEField<u32> status;
        BEField<u64> n_channels;
        BEField<u64> n_blocks;
        BEField<u32> port_size;
        BEField<u32> port_addr;
    };

    struct Port {
        u32 id = -1;
        u32 status = CELL_AUDIO_STATUS_CLOSE;
        u64 n_channels = 0;
        u64 n_blocks = 0;
        u32 addr = 0;
        u32 size = 0;
    };

    static constexpr u64 EVENT_QUEUE_KEY = 0x1234A1BE1234A1BE;
    u32 equeue_id = 0;
    Port ports[8];
    u32 read_positions_addr;
    
    std::mutex audio_mutex;
    std::thread audio_thread;
    void audioThread();
    void endAudioThread();

    u64 cellAudioCreateNotifyEventQueue();
    u64 cellAudioInit();
    u64 cellAudioSetNotifyEventQueue();
    u64 cellAudioGetPortConfig();
    u64 cellAudioPortStart();
    u64 cellAudioPortOpen();

private:
    std::atomic<bool> end_audio_thread = false;

    MAKE_LOG_FUNCTION(log, cellAudio);
};
