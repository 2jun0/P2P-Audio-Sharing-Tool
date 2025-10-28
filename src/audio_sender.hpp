#ifndef audio_sender_hpp
#define audio_sender_hpp

#include <string>
#include <gst/gst.h>

class AudioSender
{
public:
    AudioSender(const std::string &host, int port);
    ~AudioSender();

    void start();
    void stop();

private:
    std::string host;
    int port;

    GstElement *pipeline = nullptr;
    bool started = false;

    void initPipeline();
};

#endif /* audio_sender_hpp */