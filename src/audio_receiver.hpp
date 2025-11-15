#ifndef audio_receiver_hpp
#define audio_receiver_hpp

#include <string>
#include <functional>
#include <gst/gst.h>

class AudioReceiver
{
public:
    AudioReceiver(const std::string &host);
    ~AudioReceiver();

    void start();
    void stop();

    int getPort() const { return port; } // Get the actual receiving port after starting
    bool isStarted() const { return started; }

    void setOnStateUpdate(std::function<void(bool, int)> cb) { onStateUpdate = std::move(cb); }

private:
    std::string host;
    int port = -1;

    GstElement *pipeline = nullptr;
    bool started = false;
    std::function<void(bool, int)> onStateUpdate; // (receiving, port)

    void initPipeline();
};

#endif /* audio_receiver_hpp */