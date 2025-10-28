#include <stdexcept>
#include "audio_sender.hpp"

AudioSender::AudioSender(const std::string &host, int port)
    : host(host), port(port)
{
    initPipeline();
}

AudioSender::~AudioSender()
{
    stop();

    if (pipeline)
    {
        gst_object_unref(pipeline);
        pipeline = nullptr;
    }
}

void AudioSender::initPipeline()
{
    gst_init(nullptr, nullptr);

    std::string pipelineDesc = "wasapisrc loopback=true ! audioconvert ! audioresample ! opusenc ! rtpopuspay ! udpsink host=" + host + " port=" + std::to_string(port);
    pipeline = gst_parse_launch(pipelineDesc.c_str(), nullptr);
    if (!pipeline)
        throw std::runtime_error("Failed to create GStreamer pipeline");
}

void AudioSender::start()
{
    assert(pipeline && "Pipeline not initialized");
    assert(!started && "AudioSender cannot be reused");

    GstStateChangeReturn ret = gst_element_set_state(pipeline, GST_STATE_PLAYING);
    if (ret == GST_STATE_CHANGE_FAILURE)
        throw std::runtime_error("Failed to set pipeline to PLAYING state");
    
    started = true;
}

void AudioSender::stop()
{
    if (pipeline)
        gst_element_set_state(pipeline, GST_STATE_NULL);
}
