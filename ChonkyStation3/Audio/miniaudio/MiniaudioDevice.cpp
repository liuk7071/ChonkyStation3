#include "MiniaudioDevice.hpp"
#include <chrono>
#include <thread>


void MiniaudioDevice::init() {
    ma_device_config config;
    config = ma_device_config_init(ma_device_type_playback);
    config.playback.format = ma_format_f32;
    config.playback.channels = n_channels;
    config.sampleRate = 48000;
    config.pUserData = this;
    config.aaudio.usage = ma_aaudio_usage_game;

    // Audio callback
    config.dataCallback = [](ma_device* device, void* out, const void* input, ma_uint32 frame_count) {
        auto dev = (MiniaudioDevice*)device->pUserData;
        if (dev->buf.size() < dev->n_channels) return;
        
        dev->mutex.lock();
        
        const s64 expected_samples = frame_count * dev->n_channels;
        const s64 n_samples = std::min<size_t>(dev->buf.size(), expected_samples);
        const s64 missing = expected_samples - n_samples;
        
        float* out_buf = (float*)out;
        std::memset(out_buf, 0, expected_samples * sizeof(float));
        
        for (int i = 0; i < n_samples - dev->n_channels; i++) {
            *out_buf++ = dev->buf.front();
            dev->buf.pop_front();
        }
        
        // Write the last samples (1 per channel) out of the loop above to save it
        std::vector<float> last_sample;
        for (int i = 0; i < dev->n_channels; i++) {
            *out_buf++ = dev->buf.front();
            last_sample.push_back(dev->buf.front());
            dev->buf.pop_front();
        }
        
        // Fill the remaining part of the buffer with a copy of the last sample
        for (int i = 0; i < missing; i += dev->n_channels) {
            *out_buf++ = last_sample[i % dev->n_channels];
        }
        
        dev->mutex.unlock();
    };
    
    if (ma_device_init(nullptr, &config, &device) != MA_SUCCESS) {
        Helpers::panic("Failed to init miniaudio\n");
    }
    
    if (ma_device_start(&device) != MA_SUCCESS) {
        Helpers::panic("Failed to start audio device\n");
    }
    
    printf("Initialized Miniaudio device\n");
}

void MiniaudioDevice::end() {
    ma_device_uninit(&device);
}

void MiniaudioDevice::setChannels(int n_channels) {
    // TODO
}

void MiniaudioDevice::pushAudio(float* ptr, size_t n_samples) {
    std::lock_guard<std::mutex> lock(mutex);
    
    for (int i = 0; i < n_samples; i++) {
        buf.push_back(*ptr++);
    }
}
