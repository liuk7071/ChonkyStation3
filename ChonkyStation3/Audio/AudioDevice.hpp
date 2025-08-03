#pragma once

#include <common.hpp>


class AudioDevice {
public:
    virtual void init() = 0;
    virtual void end() = 0;
    virtual void setChannels(int n_channels) = 0;
    virtual void pushAudio(float* ptr, size_t n_samples) = 0;
};
