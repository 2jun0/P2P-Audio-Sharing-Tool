#ifndef audio_sender_hpp
#define audio_sender_hpp

#include <string>
#include <functional>
#include <gst/gst.h>

class AudioSender
{
public:
    AudioSender(const std::string &host, int port);
    ~AudioSender();

    void start();
    void stop();

    bool isStarted() const { return started; }

    void setOnStateUpdate(std::function<void(bool)> cb) { onStateUpdate = std::move(cb); }

private:
    std::string host;
    int port;

    GstElement *pipeline = nullptr;
    bool started = false;

    std::function<void(bool)> onStateUpdate; // started

    void initPipeline();
};

#endif /* audio_sender_hpp */