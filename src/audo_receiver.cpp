#include <stdexcept>
#include <cassert>
#include "audio_receiver.hpp"

AudioReceiver::AudioReceiver(int port) : port(port)
{
    gst_init(nullptr, nullptr);
    initPipeline();
}

AudioReceiver::~AudioReceiver()
{
    stop();

    if (pipeline) 
    {
        gst_object_unref(pipeline);
        pipeline = nullptr;
    }
}

void AudioReceiver::initPipeline()
{
    std::string pipelineDesc =
        "udpsrc port=" + std::to_string(port) +
        " caps=\"application/x-rtp, media=(string)audio, clock-rate=(int)48000, encoding-name=(string)OPUS\" ! rtpopusdepay ! opusdec ! audioconvert ! audioresample ! autoaudiosink";

    pipeline = gst_parse_launch(pipelineDesc.c_str(), nullptr);
    if (!pipeline)
        throw std::runtime_error("Failed to create GStreamer pipeline");
}

void AudioReceiver::start()
{
    assert(pipeline && "Pipeline not initialized");
    if (started)
        assert("AudioReceiver cannot be reused");

    gst_element_set_state(pipeline, GST_STATE_PLAYING);
    started = true;
}

void AudioReceiver::stop()
{
    if (pipeline)
        gst_element_set_state(pipeline, GST_STATE_NULL);
}
