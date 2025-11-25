#include "property_helper.hpp"
#include "audio_device_manager.hpp"

AudioDeviceManager::AudioDeviceManager()
{
    registerListeners();
}

AudioDeviceManager::~AudioDeviceManager()
{
    unregisterListeners();
}

void AudioDeviceManager::registerListeners()
{
    // Output device change listener
    auto address = getPropertyAddress(kAudioHardwarePropertyDefaultOutputDevice);
    OSStatus status = AudioObjectAddPropertyListener(kAudioObjectSystemObject, &address, defaultOutputDeviceChanged_ioProc, this);
    if (status != noErr)
    {
        // TODO: Error handling
    }
}

void AudioDeviceManager::unregisterListeners()
{
    // Output device change listener
    auto address = getPropertyAddress(kAudioHardwarePropertyDefaultOutputDevice);
    OSStatus status = AudioObjectRemovePropertyListener(kAudioObjectSystemObject, &address, defaultOutputDeviceChanged_ioProc, this);
    if (status != noErr)
    {
        // TODO: Error handling
    }
}

std::vector<AudioDevice> AudioDeviceManager::findAllAudioDevices()
{
    auto address = getPropertyAddress(kAudioHardwarePropertyDevices);
    UInt32 size = 0;
    OSStatus status = AudioObjectGetPropertyDataSize(kAudioObjectSystemObject, &address, 0, nullptr, &size);

    auto deviceCount = size / sizeof(AudioObjectID);
    std::vector<AudioObjectID> deviceIDs(deviceCount);
    status = AudioObjectGetPropertyData(kAudioObjectSystemObject, &address, 0, nullptr, &size, deviceIDs.data());
    if (status != noErr)
    {
        return {};
    }

    std::vector<AudioDevice> devices(deviceCount);
    for (int i = 0; i < deviceCount; ++i)
    {
        devices[i] = toAudioDevice(deviceIDs[i]);
    }
    return devices;
}

AudioDevice AudioDeviceManager::findDefaultOutputDevice()
{
    AudioObjectID defaultOutputDeviceID = kAudioObjectUnknown;
    auto address = getPropertyAddress(kAudioHardwarePropertyDefaultOutputDevice);
    UInt32 size = sizeof(AudioObjectID);
    AudioObjectGetPropertyData(kAudioObjectSystemObject, &address, 0, nullptr, &size, &defaultOutputDeviceID);

    return toAudioDevice(defaultOutputDeviceID);
}

OSStatus AudioDeviceManager::handleDefaultOutputDeviceChangedIOProc()
{
    if (defaultOutputDeviceChangeCallback)
    {
        AudioDevice device = findDefaultOutputDevice();
        defaultOutputDeviceChangeCallback(device);
    }
    return noErr;
}

AudioDevice AudioDeviceManager::toAudioDevice(AudioObjectID deviceID)
{
    auto name = getStringProperty(deviceID, kAudioObjectPropertyName);
    auto uid = getStringProperty(deviceID, kAudioDevicePropertyDeviceUID);

    AudioObjectPropertyAddress iStreamAddr = getPropertyAddress(kAudioDevicePropertyStreams, kAudioDevicePropertyScopeInput);
    UInt32 iStreamSize = 0;
    AudioObjectGetPropertyDataSize(deviceID, &iStreamAddr, 0, nullptr, &iStreamSize);
    bool hasInput = iStreamSize > 0;

    AudioObjectPropertyAddress oStreamAddr = getPropertyAddress(kAudioDevicePropertyStreams, kAudioDevicePropertyScopeOutput);
    UInt32 oStreamSize = 0;
    AudioObjectGetPropertyDataSize(deviceID, &oStreamAddr, 0, nullptr, &oStreamSize);
    bool hasOutput = oStreamSize > 0;

    return AudioDevice{name, uid, hasInput, hasOutput, static_cast<int>(deviceID)};
}

OSStatus defaultOutputDeviceChanged_ioProc(AudioObjectID, UInt32, const AudioObjectPropertyAddress *, void *inClientData)
{
    AudioDeviceManager *self = static_cast<AudioDeviceManager *>(inClientData);
    return self->handleDefaultOutputDeviceChangedIOProc();
}