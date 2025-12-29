#include <stdexcept>
#include <cassert>
#include "audio_receiver.hpp"

AudioReceiver::AudioReceiver(const std::string &host, const std::optional<AudioDevice> &outputDevice) : host(host), outputDevice(outputDevice)
{
    initPipeline();
}

AudioReceiver::~AudioReceiver()
{
    stop();
}

void AudioReceiver::initPipeline()
{
    // TODO: Filter by host address
    int _port = (port == -1) ? 0 : port;
    std::string pipelineDesc = "udpsrc name=recv_src port=" + std::to_string(_port) + " buffer-size=524288 caps=\"application/x-rtp, media=(string)audio, clock-rate=(int)48000, encoding-name=(string)OPUS, payload=(int)96\" ! rtpbin drop-on-latency=true do-lost=true ! rtpopusdepay ! opusdec plc=true ! audioconvert ! audioresample ! ";

    if (isUsingDefaultOutput())
    {
#if defined(_WIN32)
        pipelineDesc += "wasapisink low-latency=true buffer-time=20000 latency-time=5000";
#else
        pipelineDesc += "autoaudiosink";
#endif
    }
    else
    {
#if defined(_WIN32)
        pipelineDesc += "wasapisink device=\"" + outputDevice->uid + "\" low-latency=true buffer-time=20000 latency-time=5000";
#elif defined(__APPLE__)
        pipelineDesc += "osxaudiosink device=" + std::to_string(outputDevice->id);
#else
        pipelineDesc += "autoaudiosink";
#endif
    }

    pipeline = gst_parse_launch(pipelineDesc.c_str(), nullptr);
    if (!pipeline)
        throw std::runtime_error("Failed to create GStreamer pipeline");
}

void AudioReceiver::start()
{
    assert(pipeline && "Pipeline not initialized");
    assert(!started && "AudioReceiver already started");

    playPipeline();

    started = true;
    if (onStateUpdate)
        onStateUpdate(started, port, outputDevice);
}

void AudioReceiver::stop()
{
    if (pipeline)
    {
        gst_element_set_state(pipeline, GST_STATE_NULL);
        gst_object_unref(pipeline);
        pipeline = nullptr;
    }

    if (started)
    {
        started = false;
        if (onStateUpdate)
            onStateUpdate(started, port, outputDevice);
    }
}

void AudioReceiver::updateOutputDevice(const std::optional<AudioDevice> &outputDevice)
{
    this->outputDevice = outputDevice;

    // Stop pipeline
    if (pipeline)
    {
        gst_element_set_state(pipeline, GST_STATE_NULL);
        gst_object_unref(pipeline);
        pipeline = nullptr;
    }

    initPipeline();

    // Restart pipeline
    if (started)
        playPipeline();

    if (onStateUpdate)
        onStateUpdate(started, port, outputDevice);
}

void AudioReceiver::playPipeline()
{
    assert(pipeline && "Pipeline not initialized");

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

    // Play
    ret = gst_element_set_state(pipeline, GST_STATE_PLAYING);
    if (ret == GST_STATE_CHANGE_FAILURE)
        throw std::runtime_error("Failed to set pipeline to PLAYING state");

    port = actualPort;
}