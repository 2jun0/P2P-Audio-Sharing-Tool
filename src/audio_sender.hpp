#ifndef audio_sender_hpp
#define audio_sender_hpp

#include <string>
#include <gst/gst.h>

class AudioSender
{
public:
    AudioSender(const std::string &host);
    ~AudioSender();

    void start();
    void stop();
    
    int getPort() const { return port; } // Get the actual sending port after starting

private:
    std::string host;
    int port = -1;

    GstElement *pipeline = nullptr;
    bool started = false;

    void initPipeline();
};

#endif /* audio_sender_hpp */