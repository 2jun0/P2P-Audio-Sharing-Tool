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
    assert(!started && "AudioReceiver cannot be reused");

    GstStateChangeReturn ret = gst_element_set_state(pipeline, GST_STATE_PLAYING);
    if (ret == GST_STATE_CHANGE_FAILURE)
        throw std::runtime_error("Failed to set pipeline to PLAYING state");

    started = true;
}

void AudioReceiver::stop()
{
    if (pipeline)
        gst_element_set_state(pipeline, GST_STATE_NULL);
}
