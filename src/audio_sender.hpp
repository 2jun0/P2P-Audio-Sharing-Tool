#include <string>

#include <string>
#include <stdexcept>
#include <gst/gst.h>
#include <gst/app/gstappsrc.h>
#include <gst/gstcaps.h>
#include <gst/gstbuffer.h>

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