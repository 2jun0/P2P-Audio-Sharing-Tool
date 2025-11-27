#include <stdexcept>
#include <cassert>
#include "audio_sender.hpp"

#if defined(__APPLE__)
#include "ensure_loopback_bridge.hpp"
#endif

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
    std::string pipelineDesc = "";
#if defined(_WIN32)
    // WASAPI loopback
    pipelineDesc += "wasapisrc loopback=true";
#elif defined(__APPLE__)
    // Loopback aggregate device
    const std::string loopbackUID = "com.2jun0.audiosharingtool.loopback";
    const std::string loopbackName = "Audio Sharing Tool Loopback";
    if (!ensureMacosLoopbackDevice(loopbackUID, loopbackName))
        throw std::runtime_error("Not supported on this device");

    pipelineDesc += "osxaudiosrc device=" + loopbackUID;
#else
    throw std::runtime_error("Not supported on this platform");
#endif
    pipelineDesc += " ! audioconvert ! audioresample ! opusenc ! rtpopuspay ! udpsink host=" + host + " port=" + std::to_string(port);
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
    if (onStateUpdate)
        onStateUpdate(started);
}

void AudioSender::stop()
{
    started = false;

    if (pipeline)
        gst_element_set_state(pipeline, GST_STATE_NULL);

    if (onStateUpdate)
        onStateUpdate(started);
}
