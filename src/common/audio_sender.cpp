#include <stdexcept>
#include <cassert>
#include <cstring>
#include "audio_sender.hpp"
#include "audio_device_manager.hpp"

#if defined(__ANDROID__)
#include <gst/app/gstappsrc.h>
#endif

AudioSender::AudioSender(const std::string &targetHost, int targetPort, const std::optional<AudioDevice> &inputDevice)
    : targetHost(targetHost), targetPort(targetPort), inputDevice(inputDevice)
{
    initPipeline();
}

AudioSender::~AudioSender()
{
    stop();
}

void AudioSender::initPipeline()
{
    GstElement *src = nullptr;

#if defined(__ANDROID__)
    src = gst_element_factory_make("appsrc", "appsrc");
    if (!src)
        throw std::runtime_error("Failed to create appsrc element");
    g_object_set(src, "format", GST_FORMAT_TIME, "is-live", TRUE, "block", TRUE, "do-timestamp", TRUE, NULL);
#else
    if (inputDevice.has_value())
        src = AudioDeviceManager::createSourceElement(inputDevice->uid);
    if (!src)
        src = gst_element_factory_make("autoaudiosrc", nullptr);
    if (!src)
        throw std::runtime_error("Failed to create audio source element");
#endif

    GError *err = nullptr;
    GstElement *chain = gst_parse_bin_from_description(
        "audioconvert ! audioresample ! audio/x-raw,rate=48000,channels=2"
        " ! opusenc ! rtpopuspay pt=96 ! queue max-size-buffers=1 ! rtpbin"
        " ! udpsink name=udp_sink sync=false async=false",
        TRUE, &err);
    if (!chain)
    {
        gst_object_unref(src);
        std::string msg = err ? err->message : "Unknown error";
        if (err)
            g_error_free(err);
        throw std::runtime_error("Failed to create processing chain: " + msg);
    }
    if (err)
        g_error_free(err);

    GstElement *udpsink = gst_bin_get_by_name(GST_BIN(chain), "udp_sink");
    g_object_set(udpsink, "host", targetHost.c_str(), "port", targetPort, NULL);
    gst_object_unref(udpsink);

    pipeline = gst_pipeline_new("sender_pipeline");
    gst_bin_add_many(GST_BIN(pipeline), src, chain, NULL);

    if (!gst_element_link(src, chain))
        throw std::runtime_error("Failed to link audio source to processing chain");

#if defined(__ANDROID__)
    appSrc = gst_bin_get_by_name(GST_BIN(pipeline), "appsrc");
    if (!appSrc)
        throw std::runtime_error("Failed to locate appsrc in pipeline");
    gst_app_src_set_stream_type(GST_APP_SRC(appSrc), GST_APP_STREAM_TYPE_STREAM);
    gst_app_src_set_format(GST_APP_SRC(appSrc), GST_FORMAT_TIME);
#endif
}

void AudioSender::playPipeline()
{
    assert(pipeline && "Pipeline not initialized");

    GstStateChangeReturn ret = gst_element_set_state(pipeline, GST_STATE_PLAYING);
    if (ret == GST_STATE_CHANGE_FAILURE)
        throw std::runtime_error("Failed to set pipeline to PLAYING state");
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
#if defined(__ANDROID__)
    if (appSrc)
    {
        gst_object_unref(appSrc);
        appSrc = nullptr;
    }
#endif

    if (pipeline)
    {
        gst_element_set_state(pipeline, GST_STATE_NULL);
        gst_object_unref(pipeline);
        pipeline = nullptr;
    }

    started = false;
}

void AudioSender::updateInputDevice(const std::optional<AudioDevice> &inputDevice)
{
    this->inputDevice = inputDevice;

#if defined(__ANDROID__)
    if (appSrc)
    {
        gst_object_unref(appSrc);
        appSrc = nullptr;
    }
#endif

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

#if defined(__ANDROID__)
void AudioSender::pushPcmFrame(const int16_t *data, size_t frameCount, int sampleRate, int channelCount)
{
    if (!data || frameCount == 0 || sampleRate <= 0 || channelCount <= 0)
        return;

    const size_t totalSamples = frameCount * static_cast<size_t>(channelCount);
    std::scoped_lock lock(appSrcMutex);
    if (!appSrc || !started)
        return;

    if (sampleRate != currentSampleRate || channelCount != currentChannelCount)
    {
        GstCaps *caps = gst_caps_new_simple(
            "audio/x-raw",
            "format", G_TYPE_STRING, "S16LE",
            "layout", G_TYPE_STRING, "interleaved",
            "rate", G_TYPE_INT, sampleRate,
            "channels", G_TYPE_INT, channelCount,
            nullptr);
        gst_app_src_set_caps(GST_APP_SRC(appSrc), caps);
        gst_caps_unref(caps);
        currentSampleRate = sampleRate;
        currentChannelCount = channelCount;
    }

    const gsize byteSize = totalSamples * sizeof(int16_t);
    GstBuffer *buffer = gst_buffer_new_allocate(nullptr, byteSize, nullptr);
    if (!buffer)
        return;

    GstMapInfo map;
    if (!gst_buffer_map(buffer, &map, GST_MAP_WRITE))
    {
        gst_buffer_unref(buffer);
        return;
    }

    std::memcpy(map.data, data, byteSize);
    gst_buffer_unmap(buffer, &map);

    GST_BUFFER_PTS(buffer) = GST_CLOCK_TIME_NONE;
    GST_BUFFER_DURATION(buffer) = gst_util_uint64_scale_int(frameCount, GST_SECOND, sampleRate);

    const GstFlowReturn result = gst_app_src_push_buffer(GST_APP_SRC(appSrc), buffer);
    if (result != GST_FLOW_OK)
        gst_buffer_unref(buffer);
}
#endif
