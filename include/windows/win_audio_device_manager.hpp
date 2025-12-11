#ifndef audio_device_manager_hpp
#define audio_device_manager_hpp

#include "audio_device.hpp"
#include <vector>
#include <functional>
#include <mmdeviceapi.h>
#include "audio_device_notification_client.hpp"
#include <wrl/client.h>

class AudioDeviceManager
{
public:
    AudioDeviceManager();
    ~AudioDeviceManager();

    std::vector<AudioDevice> findAllAudioDevices();
    AudioDevice findDefaultOutputDevice();

    void setDefaultOutputDeviceChangeCallback(std::function<void(AudioDevice)> callback)
    {
        defaultOutputDeviceChangeCallback = callback;
    }

private:
    IMMDeviceEnumerator *enumerator = nullptr;
    std::function<void(AudioDevice)> defaultOutputDeviceChangeCallback;
    Microsoft::WRL::ComPtr<AudioDeviceNotificationClient> notificationClient;

    AudioDevice toAudioDevice(IMMDevice *device, bool isSpeaker);
};

#endif /* audio_device_manager_hpp */
