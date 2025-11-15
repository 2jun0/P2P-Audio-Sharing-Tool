#include <stdexcept>
#include <cassert>
#include "audio_receiver.hpp"

AudioReceiver::AudioReceiver(const std::string &host) : host(host)
{
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
        "udpsrc name=recv_src address=" + host + " port=0" +
        " caps=\"application/x-rtp, media=(string)audio, clock-rate=(int)48000, encoding-name=(string)OPUS\" ! rtpopusdepay ! opusdec ! audioconvert ! audioresample ! autoaudiosink";

    pipeline = gst_parse_launch(pipelineDesc.c_str(), nullptr);
    if (!pipeline)
        throw std::runtime_error("Failed to create GStreamer pipeline");
}

void AudioReceiver::start()
{
    assert(pipeline && "Pipeline not initialized");
    assert(!started && "AudioReceiver cannot be reused");

    // Ready
    GstStateChangeReturn ret = gst_element_set_state(pipeline, GST_STATE_READY);
    if (ret == GST_STATE_CHANGE_FAILURE)
        throw std::runtime_error("Failed to set pipeline to READY state");

    GstElement *udpsrc = gst_bin_get_by_name(GST_BIN(pipeline), "recv_src");
    if (!udpsrc)
        throw std::runtime_error("Failed to find udpsrc element to get port");

    int actualPort = 0;
    g_object_get(udpsrc, "port", &actualPort, NULL);
    gst_object_unref(udpsrc);
    if (actualPort <= 0)
        throw std::runtime_error("Failed to obtain bound UDP port");
    port = actualPort;

    // Play
    ret = gst_element_set_state(pipeline, GST_STATE_PLAYING);
    if (ret == GST_STATE_CHANGE_FAILURE)
        throw std::runtime_error("Failed to set pipeline to PLAYING state");

    started = true;
    if (onStateUpdate)
        onStateUpdate(started, port);
}

void AudioReceiver::stop()
{
    started = false;

    if (pipeline)
        gst_element_set_state(pipeline, GST_STATE_NULL);

    if (onStateUpdate)
        onStateUpdate(started, port);
}
