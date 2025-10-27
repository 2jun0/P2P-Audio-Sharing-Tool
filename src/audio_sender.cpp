#include <stdexcept>
#include <gst/app/gstappsrc.h>
#include <gst/gstbuffer.h>
#include "audio_sender.hpp"

AudioSender::AudioSender(const std::string &host, int port, const std::string &format, int sampleRate, int channels, const std::string &layout)
    : host(host), port(port), sampleRate(sampleRate), channels(channels)
{
    int bytesPerSample;
    if (format == "S16LE")
        bytesPerSample = 2;
    else if (format == "F32LE")
        bytesPerSample = 4;
    else if (format == "U8")
        bytesPerSample = 1;
    else
        throw std::invalid_argument("Unsupported audio format: " + format);

    blockAlign = bytesPerSample * channels;
    caps = gst_caps_new_simple("audio/x-raw",
                               "format", G_TYPE_STRING, format.c_str(),
                               "rate", G_TYPE_INT, sampleRate,
                               "channels", G_TYPE_INT, channels,
                               "layout", G_TYPE_STRING, layout.c_str());
    if (!caps)
        throw std::runtime_error("Failed to create caps");

    initPipeline();
}

AudioSender::~AudioSender()
{
    if (pipeline)
    {
        gst_element_set_state(pipeline, GST_STATE_NULL);
        gst_object_unref(pipeline);
        pipeline = nullptr;
    }
}

void AudioSender::initPipeline()
{
    gst_init(nullptr, nullptr);

    std::string pipelineDesc = "appsrc name=mysrc is-live=true format=time ! audioconvert ! audioresample ! opusenc ! rtpopuspay ! udpsink host=" + host + " port=" + std::to_string(port);
    pipeline = gst_parse_launch(pipelineDesc.c_str(), nullptr);
    if (!pipeline)
        throw std::runtime_error("Failed to create GStreamer pipeline");

    appsrc = gst_bin_get_by_name(GST_BIN(pipeline), "mysrc");
    if (!appsrc)
        throw std::runtime_error("Failed to get appsrc element");

    gst_app_src_set_stream_type(GST_APP_SRC(appsrc), GST_APP_STREAM_TYPE_STREAM);
    gst_app_src_set_format(GST_APP_SRC(appsrc), GST_FORMAT_TIME);
    gst_app_src_set_caps(GST_APP_SRC(appsrc), caps);
    gst_caps_unref(caps);

    gst_element_set_state(pipeline, GST_STATE_PLAYING);
}

void AudioSender::pushAudio(uint8_t *data, uint32_t nFrames)
{
    int bufferSize = nFrames * blockAlign;

    GstBuffer *buffer = gst_buffer_new_allocate(nullptr, bufferSize, nullptr);
    if (!buffer)
        throw std::runtime_error("Failed to allocate GetBuffer");

    GstMapInfo map;
    gst_buffer_map(buffer, &map, GST_MAP_WRITE);
    memcpy(map.data, data, bufferSize);
    gst_buffer_unmap(buffer, &map);

    totalFrames += nFrames;
    GST_BUFFER_PTS(buffer) = gst_util_uint64_scale(totalFrames, GST_SECOND, sampleRate);
    GST_BUFFER_DURATION(buffer) = gst_util_uint64_scale(nFrames, GST_SECOND, sampleRate);

    GstFlowReturn ret = gst_app_src_push_buffer(GST_APP_SRC(appsrc), buffer);
    if (ret != GST_FLOW_OK) 
    {
        gst_buffer_unref(buffer);
        throw std::runtime_error(std::string("Failed to push buffer: ") + gst_flow_get_name(ret));
    }
}
