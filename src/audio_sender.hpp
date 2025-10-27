#ifndef audio_sender_hpp
#define audio_sender_hpp

#include <string>
#include <cstdint>
#include <gst/gst.h>
#include <gst/gstcaps.h>

class AudioSender
{
public:
    AudioSender(const std::string &host, int port, const std::string &format, int sampleRate, int channels, const std::string &layout);
    ~AudioSender();

    void pushAudio(uint8_t *data, uint32_t nFrames);

private:
    std::string host;
    int port;

    GstCaps *caps = nullptr;
    GstElement *pipeline = nullptr;
    GstElement *appsrc = nullptr;

    int sampleRate;
    int channels;
    int blockAlign;

    uint64_t totalFrames = 0;

    void initPipeline();
};

#endif /* audio_sender_hpp */