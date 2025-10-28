#include "audio_streamer.hpp"

AudioStreamer::AudioStreamer(const std::string &host, int port)
{
    sender = std::make_unique<AudioSender>(host, port);
}

void AudioStreamer::start()
{
    sender->start();
}

void AudioStreamer::stop()
{
    sender->stop();
}