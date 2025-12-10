#ifndef audio_sender_hpp
#define audio_sender_hpp

#include <string>
#include <functional>
#include <gst/gst.h>
#include <optional>
#include "audio_deivce.hpp"

#if defined(__ANDROID__)
#include <cstddef>
#include <cstdint>
#include <mutex>
#endif

class AudioSender
{
public:
    AudioSender(const std::string &host, int port, const std::optional<AudioDevice> &inputDevice);
    ~AudioSender();

    void start();
    void stop();

    void updateInputDevice(const std::optional<AudioDevice> &inputDevice);

    bool isStarted() const { return started; }

    void setOnStateUpdate(std::function<void(bool, const std::optional<AudioDevice> &)> cb) { onStateUpdate = std::move(cb); }

#if defined(__ANDROID__)
    void pushPcmFrame(const int16_t *data, size_t frameCount, int sampleRate, int channelCount);
#endif

private:
    std::string host;
    int port;
    // When std::nullopt, the sender should use the platform's default input device.
    std::optional<AudioDevice> inputDevice;

    GstElement *pipeline = nullptr;
    bool started = false;
    std::function<void(bool, const std::optional<AudioDevice> &)> onStateUpdate; // started, inputDevice

    void initPipeline();
    void playPipeline();

#if defined(__ANDROID__)
    GstElement *appSrc = nullptr;
    int currentSampleRate = 0;
    int currentChannelCount = 0;
    mutable std::mutex appSrcMutex;
#endif
};

#endif /* audio_sender_hpp */