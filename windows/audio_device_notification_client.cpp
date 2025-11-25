#include "audio_device_notification_client.hpp"
#include <functiondiscoverykeys_devpkey.h>
#include "property_helper.hpp"

AudioDeviceNotificationClient::AudioDeviceNotificationClient(std::function<void(LPCWSTR, bool)> callback)
    : refCount(1), onDefaultDeviceChanged(callback) {}

HRESULT STDMETHODCALLTYPE AudioDeviceNotificationClient::QueryInterface(REFIID riid, void **ppvObject)
{
    if (riid == IID_IUnknown || riid == __uuidof(IMMNotificationClient))
    {
        *ppvObject = static_cast<IMMNotificationClient *>(this);
        AddRef();
        return S_OK;
    }
    *ppvObject = nullptr;
    return E_NOINTERFACE;
}

ULONG STDMETHODCALLTYPE AudioDeviceNotificationClient::AddRef()
{
    return InterlockedIncrement(&refCount);
}

ULONG STDMETHODCALLTYPE AudioDeviceNotificationClient::Release()
{
    ULONG count = InterlockedDecrement(&refCount);
    if (count == 0)
        delete this;
    return count;
}

HRESULT STDMETHODCALLTYPE AudioDeviceNotificationClient::OnDefaultDeviceChanged(EDataFlow flow, ERole role, LPCWSTR pwstrDefaultDeviceId)
{
    if (role == eConsole && pwstrDefaultDeviceId)
    {
        if (onDefaultDeviceChanged)
        {
            if (flow == eRender)
            {
                onDefaultDeviceChanged(pwstrDefaultDeviceId, true);
            }
            else if (flow == eCapture)
            {
                onDefaultDeviceChanged(pwstrDefaultDeviceId, false);
            }
        }
    }
    return S_OK;
}