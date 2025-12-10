#ifndef audio_device_notification_client_hpp
#define audio_device_notification_client_hpp

#include <functional>
#include <string>
#include <windows.h>
#include <mmdeviceapi.h>

class AudioDeviceNotificationClient : public IMMNotificationClient
{
public:
    AudioDeviceNotificationClient(std::function<void(LPCWSTR, bool)> callback);

    HRESULT STDMETHODCALLTYPE QueryInterface(REFIID riid, void **ppvObject) override;
    ULONG STDMETHODCALLTYPE AddRef() override;
    ULONG STDMETHODCALLTYPE Release() override;

    HRESULT STDMETHODCALLTYPE OnDefaultDeviceChanged(EDataFlow flow, ERole role, LPCWSTR pwstrDefaultDeviceId) override;
    HRESULT STDMETHODCALLTYPE OnDeviceAdded(LPCWSTR) override { return S_OK; }
    HRESULT STDMETHODCALLTYPE OnDeviceRemoved(LPCWSTR) override { return S_OK; }
    HRESULT STDMETHODCALLTYPE OnDeviceStateChanged(LPCWSTR, DWORD) override { return S_OK; }
    HRESULT STDMETHODCALLTYPE OnPropertyValueChanged(LPCWSTR, const PROPERTYKEY) override { return S_OK; }

private:
    LONG refCount;
    std::function<void(LPCWSTR, bool)> onDefaultDeviceChanged;
};

#endif /* audio_device_notification_client_hpp */