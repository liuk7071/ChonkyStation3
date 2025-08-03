#include "AudioDevice.hpp"


class NullDevice : public AudioDevice {
public:
    void init() override;
    void end() override;
    void setChannels(int n_channels) override;
    void pushAudio(float* ptr, size_t n_samples) override;
};
