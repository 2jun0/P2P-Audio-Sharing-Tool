#ifndef audio_device_manaper_hpp
#define audio_device_manaper_hpp

#include <CoreAudio/AudioHardware.h>
#include "audio_deivce.hpp"
#include <vector>
#include <functional>

class AudioDeviceManager
{
public:
    AudioDeviceManager();
    ~AudioDeviceManager();

    std::vector<AudioDevice> findAllAudioDevices();
    AudioDevice findDefaultOutputDevice();

    OSStatus handleDefaultOutputDeviceChangedIOProc();
    void setDefaultOutputDeviceChangeCallback(std::function<void(AudioDevice)> callback)
    {
        defaultOutputDeviceChangeCallback = callback;
    }

private:
    std::function<void(AudioDevice)> defaultOutputDeviceChangeCallback;

    void registerListeners();
    void unregisterListeners();
    AudioDevice toAudioDevice(AudioObjectID deviceID);
};

static OSStatus defaultOutputDeviceChanged_ioProc(AudioObjectID inObjectID,
                                                  UInt32 inNumberAddresses,
                                                  const AudioObjectPropertyAddress *inAddresses,
                                                  void *inClientData);

#endif /* audio_device_manaper_hpp */
