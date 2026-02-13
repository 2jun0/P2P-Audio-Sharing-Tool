#include "win_audio_device_manager.hpp"
#include <Audioclient.h>
#include <functiondiscoverykeys_devpkey.h>
#include "property_helper.hpp"
#include <stdexcept>

using Microsoft::WRL::ComPtr;

AudioDeviceManager::AudioDeviceManager()
{
    CoInitialize(NULL);
    HRESULT hr = CoCreateInstance(__uuidof(MMDeviceEnumerator), NULL, CLSCTX_ALL,
                                  IID_PPV_ARGS(&enumerator));
    if (FAILED(hr) || !enumerator)
        throw std::runtime_error("Failed to create MMDeviceEnumerator");

    auto client = new AudioDeviceNotificationClient([this](LPCWSTR deviceId, bool isSpeaker)
                                                    {
        if (enumerator && defaultOutputDeviceChangeCallback)
        {
            ComPtr<IMMDevice> device;
            HRESULT hr2 = enumerator->GetDevice(deviceId, &device);
            if (SUCCEEDED(hr2) && device)
            {
                AudioDevice audioDevice = toAudioDevice(device.Get(), isSpeaker);
                defaultOutputDeviceChangeCallback(audioDevice);
            }
        } });
    notificationClient.Attach(client);

    if (enumerator)
        enumerator->RegisterEndpointNotificationCallback(notificationClient.Get());
}

AudioDeviceManager::~AudioDeviceManager()
{
    if (enumerator)
    {
        enumerator->UnregisterEndpointNotificationCallback(notificationClient.Get());
    }

    enumerator.Reset();
    notificationClient.Reset();
    CoUninitialize();
}

std::vector<AudioDevice> AudioDeviceManager::findAllAudioDevices()
{
    std::vector<AudioDevice> result;
    if (!enumerator)
        throw std::runtime_error("Audio device enumerator is not initialized");

    ComPtr<IMMDeviceCollection> collection;
    HRESULT hr = enumerator->EnumAudioEndpoints(eRender, DEVICE_STATE_ACTIVE, &collection);
    if (FAILED(hr) || !collection)
        throw std::runtime_error("Failed to enumerate audio devices");

    UINT count = 0;
    collection->GetCount(&count);

    for (UINT i = 0; i < count; ++i)
    {
        ComPtr<IMMDevice> device;
        collection->Item(i, &device);

        try
        {
            AudioDevice audioDevice = toAudioDevice(device.Get(), true);
            result.push_back(audioDevice);
        }
        catch (...)
        {
            // ignore error
            // TODO: logging?
        }
    }

    return result;
}

AudioDevice AudioDeviceManager::findDefaultOutputDevice()
{
    if (!enumerator)
        throw std::runtime_error("Audio device enumerator is not initialized");

    ComPtr<IMMDevice> device;
    HRESULT hr = enumerator->GetDefaultAudioEndpoint(eRender, eConsole, &device);
    if (FAILED(hr) || !device)
        throw std::runtime_error("Failed to get default output device");

    return toAudioDevice(device.Get(), true);
}

AudioDevice AudioDeviceManager::toAudioDevice(IMMDevice *device, bool isSpeaker)
{
    LPWSTR id = nullptr;
    ComPtr<IPropertyStore> props;
    PROPVARIANT name;
    PropVariantInit(&name);

    try
    {
        // GUID
        HRESULT hr = device->GetId(&id);
        if (FAILED(hr))
            throw std::runtime_error("Failed to get device ID: HRESULT = 0x" + std::to_string(hr));
        std::string idStr = wideToUtf8(id);

        // Property Store
        hr = device->OpenPropertyStore(STGM_READ, &props);
        if (FAILED(hr))
            throw std::runtime_error("Failed to open property store: HRESULT = 0x" + std::to_string(hr));

        // Name
        hr = props->GetValue(PKEY_Device_FriendlyName, &name);
        if (FAILED(hr))
            throw std::runtime_error("Failed to get device name: HRESULT = 0x" + std::to_string(hr));
        if (name.vt != VT_LPWSTR || name.pwszVal == nullptr)
            throw std::runtime_error("Device name has invalid type or is null");
        std::string nameStr = wideToUtf8(name.pwszVal);

        // Cleanup
        PropVariantClear(&name);
        if (id)
        {
            CoTaskMemFree(id);
            id = nullptr;
        }

        return AudioDevice(nameStr, idStr, !isSpeaker, isSpeaker);
    }
    catch (...)
    {
        PropVariantClear(&name);
        if (id)
        {
            CoTaskMemFree(id);
            id = nullptr;
        }
        throw;
    }
}
