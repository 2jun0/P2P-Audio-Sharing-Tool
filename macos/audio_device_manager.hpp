#include <CoreAudio/AudioHardware.h>
#include "audio_deivce.hpp"
#include <vector>

typedef void (*DefaultOutputDeviceChangeCallback)(AudioDevice device);

class AudioDeviceManager
{
public:
    AudioDeviceManager();
    ~AudioDeviceManager();

    std::vector<AudioDevice> findAllAudioDevices();
    AudioDevice findDefaultOutputDevice();

    OSStatus handleDefaultOutputDeviceChangedIOProc();
    void setDefaultOutputDeviceChangeCallback(DefaultOutputDeviceChangeCallback callback)
    {
        defaultOutputDeviceChangeCallback = callback;
    }

private:
    DefaultOutputDeviceChangeCallback defaultOutputDeviceChangeCallback;

    void registerListeners();
    void unregisterListeners();
    AudioDevice toAudioDevice(AudioObjectID deviceID);
};

static OSStatus defaultOutputDeviceChanged_ioProc(AudioObjectID inObjectID,
                                                  UInt32 inNumberAddresses,
                                                  const AudioObjectPropertyAddress *inAddresses,
                                                  void *inClientData);
