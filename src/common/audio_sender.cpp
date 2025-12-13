#include <stdexcept>
#include <cassert>
#include <cstring>
#include "audio_sender.hpp"

#if defined(__ANDROID__)
#include <gst/app/gstappsrc.h>
#endif

AudioSender::AudioSender(const std::string &host, int port, const std::optional<AudioDevice> &inputDevice)
    : host(host), port(port), inputDevice(inputDevice)
{
    initPipeline();
}

AudioSender::~AudioSender()
{
    stop();
}

void AudioSender::initPipeline()
{
    std::string pipelineDesc = "";

    if (inputDevice.has_value())
    {
#if defined(_WIN32)
        pipelineDesc += "wasapisrc device=\"" + inputDevice->uid + "\" low-latency=true buffer-time=20000 latency-time=5000 do-timestamp=true";
#elif defined(__APPLE__)
        pipelineDesc += "osxaudiosrc device=" + std::to_string(inputDevice->id);
#elif defined(__ANDROID__)
        throw std::runtime_error("Android does not support input audio device");
#else
        throw std::runtime_error("Not supported on this platform");
#endif
    }
    else
    {
#if defined(_WIN32)
        pipelineDesc += "wasapisrc loopback=true low-latency=true buffer-time=20000 latency-time=5000 do-timestamp=true";
#elif defined(__APPLE__)
        throw std::runtime_error("macOS requires a loopback / input audio device");
#elif defined(__ANDROID__)
        pipelineDesc += "appsrc name=appsrc format=time is-live=true block=true do-timestamp=true";
#else
        throw std::runtime_error("Not supported on this platform");
#endif
    }
    pipelineDesc += " ! audioconvert ! audioresample ! opusenc audio-type=restricted-lowdelay frame-size=10 ! rtpopuspay ! udpsink host=" + host + " port=" + std::to_string(port);
    pipeline = gst_parse_launch(pipelineDesc.c_str(), nullptr);

    if (!pipeline)
        throw std::runtime_error("Failed to create GStreamer pipeline");

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

    // Play
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
    if (onStateUpdate)
        onStateUpdate(started, inputDevice);
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

    if (started)
    {
        started = false;
        if (onStateUpdate)
            onStateUpdate(started, inputDevice);
    }
}

void AudioSender::updateInputDevice(const std::optional<AudioDevice> &inputDevice)
{
    this->inputDevice = inputDevice;

    // Stop pipeline
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

    // Restart pipeline
    if (started)
        playPipeline();

    if (onStateUpdate)
        onStateUpdate(started, inputDevice);
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
