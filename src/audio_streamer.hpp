#ifndef audio_streamer_hpp
#define audio_streamer_hpp

#include <string>
#include <memory>
#include "audio_sender.hpp"

class AudioStreamer
{
public:
    AudioStreamer(const std::string &host, int port);

    void start();
    void stop();

private:
    std::unique_ptr<AudioSender> sender;
};

#endif /* audio_streamer_hpp */