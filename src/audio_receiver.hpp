#ifndef audio_receiver_hpp
#define audio_receiver_hpp

#include <string>
#include <optional>
#include <functional>
#include <gst/gst.h>
#include "audio_deivce.hpp"

class AudioReceiver
{
public:
    AudioReceiver(const std::string &host, const std::optional<AudioDevice> &outputDevice = std::nullopt);
    ~AudioReceiver();

    void start();
    void stop();

    void updateOutputDevice(const std::optional<AudioDevice> &outputDevice);
    bool isUsingDefaultOutput() const { return !outputDevice.has_value(); }

    int getPort() const { return port; } // Get the actual receiving port after starting
    bool isStarted() const { return started; }

    void setOnStateUpdate(std::function<void(bool, int, const std::optional<AudioDevice> &)> cb) { onStateUpdate = std::move(cb); }

private:
    std::string host;
    int port = -1;
    // When std::nullopt, the receiver should use the platform's default output device.
    std::optional<AudioDevice> outputDevice;

    GstElement *pipeline = nullptr;
    bool started = false;
    std::function<void(bool, int, const std::optional<AudioDevice> &)> onStateUpdate; // (receiving, port, outputDevice)

    void initPipeline();
    void playPipeline();
};

#endif /* audio_receiver_hpp */