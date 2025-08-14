#include "CellAudio.hpp"
#include "PlayStation3.hpp"
#include <Lv2Objects/Lv2EventQueue.hpp>


void CellAudio::audioThread() {
    while (true) {
        if (end_audio_thread) break;
        if (ps3->settings.debug.dont_step_cellaudio_port_read_idx) {
            std::this_thread::sleep_for(std::chrono::milliseconds(1000));
            continue;
        }
        
        audio_mutex.lock();
        
        int sampled = 0;
        // TODO: What happens when different ports have a different number of channels?
        const size_t block_size = 256 * ports[0].n_channels * sizeof(float);
        float* buf = new float[block_size / sizeof(float)];
        std::memset(buf, 0, block_size);
        
        for (int i = 0; i < 8; i++) {
            if (ports[i].status != CELL_AUDIO_STATUS_RUN) {
                continue;
            }
            
            // Get current block
            float* curr_buf = (float*)ps3->mem.getPtr(ports[i].addr + ports[i].idx * block_size);
            
            for (int i = 0; i < block_size / sizeof(float); i++) {
                u32 val = Helpers::bswap<u32>(reinterpret_cast<u32&>(curr_buf[i]));
                buf[i] += reinterpret_cast<float&>(val);
            }
            sampled++;
            
            // Increment read idx
            ports[i].idx++;
            ports[i].idx %= ports[i].n_blocks;
            ps3->mem.write<u64>(read_positions_addr + i * sizeof(u64), ports[i].idx);
            
            // Send aftermix event
            if (equeue_id) {
                ps3->lv2_obj.get<Lv2EventQueue>(equeue_id)->send({ CellAudio::EVENT_QUEUE_KEY, 0, 0, 0 });
            }
        }
        
        for (int i = 0; i < block_size / sizeof(float); i++) {
            buf[i] /= (float)sampled;
        }
        
        // Mix and dump to file (for now)
        //std::ofstream sample("sample.bin", std::ios::binary | std::ios::app);
        //sample.write((const char*)buf, block_size);
        
        // No point in pushing audio at all if we dont step the port...
        if (!ps3->settings.debug.dont_step_cellaudio_port_read_idx)
            ps3->audio->pushAudio(buf, block_size / sizeof(float));
        
        // Sleep
        audio_mutex.unlock();
        std::this_thread::sleep_for(std::chrono::microseconds(5300));   // 5.3ms
    }
}

void CellAudio::endAudioThread() {
    end_audio_thread = true;
    if (audio_thread.joinable())
        audio_thread.join();
}

u64 CellAudio::cellAudioCreateNotifyEventQueue() {
    const u32 equeue_id_ptr = ARG0;
    const u32 key_ptr = ARG1;   // key is u64
    log("cellAudioCreateNotifyEventQueue(equeue_id_ptr: 0x%08x, key_ptr: 0%08x)\n", equeue_id_ptr, key_ptr);
    const std::scoped_lock<std::mutex> lock(audio_mutex);
    
    Lv2EventQueue* equeue = ps3->lv2_obj.create<Lv2EventQueue>();
    equeue_id = equeue->handle();
    
    ps3->mem.write<u32>(equeue_id_ptr, equeue_id);
    ps3->mem.write<u64>(key_ptr, EVENT_QUEUE_KEY);
    return CELL_OK;
}

u64 CellAudio::cellAudioPortClose() {
    u32 port_num = ARG0;
    log("cellAudioPortClose(port_num: %d)\n", port_num);
    const std::scoped_lock<std::mutex> lock(audio_mutex);
    
    ports[port_num].status = CELL_AUDIO_STATUS_CLOSE;
    return CELL_OK;
}

u64 CellAudio::cellAudioInit() {
    log("cellAudioInit()\n");
    
    // TLOU and Sonic the Hedgehog do this
    if (initialized) {
        return CELL_AUDIO_ERROR_ALREADY_INIT;
    }
    
    initialized = true;
    read_positions_addr = ps3->mem.alloc(8 * sizeof(u64), 0, true)->vaddr;
    audio_thread = std::thread(&CellAudio::audioThread, this);
    
    return CELL_OK;
}

u64 CellAudio::cellAudioSetNotifyEventQueue() {
    const u64 key = ARG0;
    log("cellAudioSetNotifyEventQueue(key: 0x%016llx)\n", key);
    //Helpers::debugAssert(key == EVENT_QUEUE_KEY, "TODO: cellAudioSetNotifyEventQueue with custom event queue\n");
    const std::scoped_lock<std::mutex> lock(audio_mutex);
    
    return CELL_OK;
}

u64 CellAudio::cellAudioGetPortConfig() {
    u32 port_num = ARG0;
    u32 config_ptr = ARG1;
    log("cellAudioGetPortConfig(port_num: %d, config_ptr: 0x%08x)\n", port_num, config_ptr);
    const std::scoped_lock<std::mutex> lock(audio_mutex);

    Helpers::debugAssert(port_num < 8, "cellAudioGetPortConfig: port_num is invalid (%d)\n", port_num);
    Port& port = ports[port_num];
    CellAudioPortConfig* config = (CellAudioPortConfig*)ps3->mem.getPtr(config_ptr);
    
    config->read_idx_addr = read_positions_addr + port_num * sizeof(u64);
    config->status = port.status;
    config->n_channels = port.n_channels;
    config->n_blocks = port.n_blocks;
    config->port_size = port.size;
    config->port_addr = port.addr;

    return CELL_OK;
}

u64 CellAudio::cellAudioAddData() {
    const u32 port_num = ARG0;
    const u32 src_ptr = ARG1;
    const u32 n_samples = ARG2;
    const float volume = ARG3;
    log("cellAudioAddData(port_num: %d, src_ptr: 0x%08x, n_samples: %d, volume: %f)\n", port_num, src_ptr, n_samples, volume);
    const std::scoped_lock<std::mutex> lock(audio_mutex);
    
    const size_t block_size = 256 * ports[port_num].n_channels * sizeof(float);
    float* src = (float*)ps3->mem.getPtr(src_ptr);
    float* dst = (float*)ps3->mem.getPtr(ports[port_num].addr + ports[port_num].idx * block_size);
    for (int i = 0; i < n_samples * ports[port_num].n_channels; i++)
        *dst++ = *src++;
    
    return CELL_OK;
}

u64 CellAudio::cellAudioPortStart() {
    const u32 port_num = ARG0;
    log("cellAudioPortStart(port_num: %d)\n", port_num);
    const std::scoped_lock<std::mutex> lock(audio_mutex);
    
    Helpers::debugAssert(port_num < 8, "cellAudioPortStart: port_num is invalid (%d)\n", port_num);
    Port& port = ports[port_num];
    port.status = CELL_AUDIO_STATUS_RUN;

    return CELL_OK;
}

u64 CellAudio::cellAudioPortOpen() {
    const u32 param_ptr = ARG0;
    const u32 port_ptr = ARG1;
    log("cellAudioPortOpen(param_ptr: 0x%08x, port_ptr: 0x%08x)\n", param_ptr, port_ptr);
    const std::scoped_lock<std::mutex> lock(audio_mutex);

    CellAudioPortParam* param = (CellAudioPortParam*)ps3->mem.getPtr(param_ptr);

    // Find an available port and open it
    u32 port_num;
    Port* port = nullptr;
    for (port_num = 0; port_num < 8; port_num++) {
        if (ports[port_num].status == CELL_AUDIO_STATUS_CLOSE) {
            port = &ports[port_num];
            port->status = CELL_AUDIO_STATUS_READY;
            port->n_channels = param->n_channels;
            port->n_blocks = param->n_blocks;
            break;
        }
    }

    if (!port) {    // No port was available
        Helpers::panic("cellAudioPortOpen: no ports available\n");
    }

    // Allocate memory
    port->size = port->n_channels * port->n_blocks * 256 * sizeof(float);
    port->addr = ps3->mem.alloc(port->size, 0, true)->vaddr;

    ps3->mem.write<u32>(port_ptr, port->id);
    log("Opened port %d (channels: %d, blocks: %d)\n", port->id, port->n_channels, port->n_blocks);

    // TODO: Do this properly later
    ps3->audio->setChannels(port->n_channels);
    
    return CELL_OK;
}
