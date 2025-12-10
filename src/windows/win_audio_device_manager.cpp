#include "win_audio_device_manager.hpp"
#include <Audioclient.h>
#include <functiondiscoverykeys_devpkey.h>
#include "property_helper.hpp"
#include <stdexcept>

AudioDeviceManager::AudioDeviceManager()
{
    CoInitialize(NULL);
    HRESULT hr = CoCreateInstance(__uuidof(MMDeviceEnumerator), NULL, CLSCTX_ALL,
                                  __uuidof(IMMDeviceEnumerator), (void **)&enumerator);

    auto client = new AudioDeviceNotificationClient([this](LPCWSTR deviceId, bool isSpeaker)
                                                    {
        if (enumerator && defaultOutputDeviceChangeCallback)
        {
            IMMDevice *device = nullptr;
            HRESULT hr2 = enumerator->GetDevice(deviceId, &device);
            if (SUCCEEDED(hr2) && device)
            {
                AudioDevice audioDevice = toAudioDevice(device, isSpeaker);
                defaultOutputDeviceChangeCallback(audioDevice);
                device->Release();
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
        enumerator->Release();
        enumerator = nullptr;
    }

    notificationClient.Reset();
    CoUninitialize();
}

std::vector<AudioDevice> AudioDeviceManager::findAllAudioDevices()
{
    std::vector<AudioDevice> result;

    if (!enumerator)
        return result;

    IMMDeviceCollection *collection = nullptr;
    HRESULT hr = enumerator->EnumAudioEndpoints(eRender, DEVICE_STATE_ACTIVE, &collection);
    if (FAILED(hr) || !collection)
        throw std::runtime_error("Failed to enumerate audio devices");

    UINT count = 0;
    collection->GetCount(&count);

    for (UINT i = 0; i < count; ++i)
    {
        IMMDevice *device = nullptr;
        collection->Item(i, &device);

        try
        {
            AudioDevice audioDevice = toAudioDevice(device, true);
            result.push_back(audioDevice);
        }
        catch (...)
        {
            // ignore error
            // TODO: logging?
        }

        device->Release();
    }

    collection->Release();
    return result;
}

AudioDevice AudioDeviceManager::toAudioDevice(IMMDevice *device, bool isSpeaker)
{
    LPWSTR id = nullptr;
    IPropertyStore *props = nullptr;
    PROPVARIANT name;
    PropVariantInit(&name);

    try
    {
        // GUID
        HRESULT hr = device->GetId(&id);
        if (FAILED(hr))
            throw std::runtime_error("Failed to get device ID: HRESULT = 0x" +
                                     std::to_string(hr));
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
        if (props)
        {
            props->Release();
            props = nullptr;
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
        if (props)
        {
            props->Release();
            props = nullptr;
        }

        throw;
    }
}
