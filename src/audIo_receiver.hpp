#ifndef audio_receiver_hpp
#define audio_receiver_hpp

#include <string>
#include <gst/gst.h>

class AudioReceiver
{
public:
    AudioReceiver(int port);
    ~AudioReceiver();

    void start();
    void stop();

private:
    int port;

    GstElement *pipeline = nullptr;
    bool started = false;

    void initPipeline();
};

#endif /* audio_receiver_hpp */