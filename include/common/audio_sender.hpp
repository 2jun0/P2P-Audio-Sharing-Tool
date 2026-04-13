#ifndef audio_sender_hpp
#define audio_sender_hpp

#include <string>
#include <functional>
#include <gst/gst.h>
#include <optional>
#include "audio_device.hpp"

#if defined(__ANDROID__)
#include <cstddef>
#include <cstdint>
#include <mutex>
#endif

class AudioSender
{
public:
    AudioSender(const std::string &targetHost, int targetPort, const std::optional<AudioDevice> &inputDevice = std::nullopt);
    ~AudioSender();

    void start();
    void stop();

    void updateInputDevice(const std::optional<AudioDevice> &inputDevice);

    bool isStarted() const { return started; }

#if defined(__ANDROID__)
    void pushPcmFrame(const int16_t *data, size_t frameCount, int sampleRate, int channelCount);
#endif

private:
    std::string targetHost;
    int targetPort;
    std::optional<AudioDevice> inputDevice;

    GstElement *pipeline = nullptr;
    bool started = false;

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