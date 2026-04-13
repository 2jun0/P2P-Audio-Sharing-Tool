#ifndef audio_device_manager_hpp
#define audio_device_manager_hpp

#include <vector>
#include <string>
#include <gst/gst.h>
#include "audio_device.hpp"

class AudioDeviceManager
{
public:
    AudioDeviceManager() = default;
    ~AudioDeviceManager() = default;

    std::vector<AudioDevice> findAllAudioDevices();

    static GstElement *createSourceElement(const std::string &uid);
    static GstElement *createSinkElement(const std::string &uid);
};

#endif // audio_device_manager_hpp
