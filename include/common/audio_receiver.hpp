#ifndef audio_receiver_hpp
#define audio_receiver_hpp

#include <string>
#include <optional>
#include <gst/gst.h>
#include "audio_device.hpp"

class AudioReceiver
{
public:
    AudioReceiver(int listenPort = 0, const std::optional<AudioDevice> &outputDevice = std::nullopt);
    ~AudioReceiver();

    void start();
    void stop();

    void updateOutputDevice(const std::optional<AudioDevice> &outputDevice);

    int getPort() const { return port; }
    bool isStarted() const { return started; }

private:
    int port;
    std::optional<AudioDevice> outputDevice;

    GstElement *pipeline = nullptr;
    bool started = false;

    void initPipeline();
    void playPipeline();
};

#endif /* audio_receiver_hpp */