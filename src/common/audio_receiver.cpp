#include <stdexcept>
#include <cassert>
#include "audio_receiver.hpp"
#include "audio_device_manager.hpp"

AudioReceiver::AudioReceiver(int listenPort, const std::optional<AudioDevice> &outputDevice)
    : port(listenPort), outputDevice(outputDevice)
{
    initPipeline();
}

AudioReceiver::~AudioReceiver()
{
    stop();
}

void AudioReceiver::initPipeline()
{
    int bindPort = (port <= 0) ? 0 : port;

    GError *err = nullptr;
    std::string chainDesc =
        "udpsrc name=recv_src port=" + std::to_string(bindPort) +
        " caps=\"application/x-rtp, media=(string)audio, clock-rate=(int)48000,"
        " encoding-name=(string)OPUS, payload=(int)96\""
        " ! rtpbin drop-on-latency=true do-lost=true"
        " ! rtpopusdepay ! opusdec plc=true"
        " ! audioconvert ! audioresample ! audio/x-raw,rate=48000,channels=2";

    GstElement *chain = gst_parse_bin_from_description(chainDesc.c_str(), TRUE, &err);
    if (!chain)
    {
        std::string msg = err ? err->message : "Unknown error";
        if (err)
            g_error_free(err);
        throw std::runtime_error("Failed to create receive chain: " + msg);
    }
    if (err)
        g_error_free(err);

    GstElement *sink = nullptr;
    if (outputDevice.has_value())
        sink = AudioDeviceManager::createSinkElement(outputDevice->uid);
    if (!sink)
        sink = gst_element_factory_make("autoaudiosink", nullptr);
    if (!sink)
    {
        gst_object_unref(chain);
        throw std::runtime_error("Failed to create audio sink element");
    }

    pipeline = gst_pipeline_new("receiver_pipeline");
    gst_bin_add_many(GST_BIN(pipeline), chain, sink, NULL);

    if (!gst_element_link(chain, sink))
        throw std::runtime_error("Failed to link receive chain to audio sink");
}

void AudioReceiver::start()
{
    assert(pipeline && "Pipeline not initialized");
    assert(!started && "AudioReceiver already started");

    playPipeline();
    started = true;
}

void AudioReceiver::stop()
{
    if (pipeline)
    {
        gst_element_set_state(pipeline, GST_STATE_NULL);
        gst_object_unref(pipeline);
        pipeline = nullptr;
    }

    started = false;
}

void AudioReceiver::updateOutputDevice(const std::optional<AudioDevice> &outputDevice)
{
    this->outputDevice = outputDevice;

    if (pipeline)
    {
        gst_element_set_state(pipeline, GST_STATE_NULL);
        gst_object_unref(pipeline);
        pipeline = nullptr;
    }

    initPipeline();

    if (started)
        playPipeline();
}

void AudioReceiver::playPipeline()
{
    assert(pipeline && "Pipeline not initialized");

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

    ret = gst_element_set_state(pipeline, GST_STATE_PLAYING);
    if (ret == GST_STATE_CHANGE_FAILURE)
        throw std::runtime_error("Failed to set pipeline to PLAYING state");

    port = actualPort;
}