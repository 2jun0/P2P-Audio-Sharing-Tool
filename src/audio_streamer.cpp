#include "audio_streamer.hpp"

AudioStreamer::AudioStreamer(const std::string &host, int port)
{
    capturer = std::make_unique<AudioCapturer>();

    std::string format = capturer->getFormat();
    int sampleRate = capturer->getSampleRate();
    int channels = capturer->getChannels();
    std::string layout = capturer->getLayout();

    sender = std::make_unique<AudioSender>(host, port, format, sampleRate, channels, layout);
    capturer->onReadAudioBuffer = [this](uint8_t* data, uint32_t nFrames) 
    {
        sender->pushAudio(data, nFrames);
    };
}

void AudioStreamer::start()
{
    capturer->start();
}

void AudioStreamer::stop()
{
    capturer->stop();
}