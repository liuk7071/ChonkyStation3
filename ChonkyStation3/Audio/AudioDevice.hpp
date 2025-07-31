#pragma once

#include <common.hpp>


class AudioDevice {
public:
    virtual void init() { Helpers::panic("Audio device backend did not define init function\n"); }
    virtual void setChannels(int n_channels) { Helpers::panic("Audio device backend did not define setChannels function\n"); }
    virtual void pushAudio(float* ptr, size_t n_samples) { Helpers::panic("Audio device backend did not define pushAudio function\n"); }
};
