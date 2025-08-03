#include "AudioDevice.hpp"

#include "miniaudio.h"

#include <deque>
#include <mutex>


class MiniaudioDevice : public AudioDevice {
public:
    void init() override;
    void end() override;
    void setChannels(int n_channels) override;
    void pushAudio(float* ptr, size_t n_samples) override;
    
private:
    ma_device device;
    ma_context context;
    
    int n_channels = 2; // TODO: Don't hardcode
    std::deque<float> buf;
    
    std::mutex mutex;
};
