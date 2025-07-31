#include "MiniaudioDevice.hpp"


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
        std::lock_guard<std::mutex> lock(dev->mutex);
        
        const auto n_samples = std::min<size_t>(dev->buf.size(), frame_count * dev->n_channels);
        float* out_buf = (float*)out;
        std::memset(out_buf, 0, frame_count * dev->n_channels * sizeof(float));
        
        for (int i = 0; i < n_samples; i++) {
            *out_buf++ = dev->buf.front();
            dev->buf.pop_front();
        }
    };
    
    if (ma_device_init(nullptr, &config, &device) != MA_SUCCESS) {
        Helpers::panic("Failed to init miniaudio\n");
    }
    
    if (ma_device_start(&device) != MA_SUCCESS) {
        Helpers::panic("Failed to start audio device\n");
    }
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
