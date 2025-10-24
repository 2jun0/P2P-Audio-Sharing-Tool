#include "audio_capturer.hpp"

AudioCapturer::AudioCapturer()
{
    
}

AudioCapturer::~AudioCapturer()
{
    // TODO: 자원 해제하기 전에 스레드 닫기

    if (pAudioClient)
        pAudioClient->Stop();
    if (pwfx)
        CoTaskMemFree(pwfx);
    if (pCaptureClient)
        pCaptureClient->Release();
    if (pAudioClient)
        pAudioClient->Release();
    if (pDevice)
        pDevice->Release();
    if (pEnumerator)
        pEnumerator->Release();
    if (hEvent)
        CloseHandle(hEvent);

    CoUninitialize();
}

void AudioCapturer::initAudioDevice()
{
    HRESULT hr;

    hr = CoInitializeEx(nullptr, COINIT_MULTITHREADED);
    if (FAILED(hr))
        throw std::runtime_error("CoInitializeEx failed: " + hresultToString(hr));

    // 오디오 디바이스 초기화
    hr = CoCreateInstance(__uuidof(MMDeviceEnumerator), nullptr, CLSCTX_ALL,
                     __uuidof(IMMDeviceEnumerator), (void **)&pEnumerator);
    if (FAILED(hr))
        throw std::runtime_error("Failed to create MMDeviceEnumerator: " + hresultToString(hr));

    hr = pEnumerator->GetDefaultAudioEndpoint(eRender, eConsole, &pDevice);
    if (FAILED(hr))
        throw std::runtime_error("Failed to get default audio endpoint: " + hresultToString(hr));

    hr = pDevice->Activate(__uuidof(IAudioClient), CLSCTX_ALL, nullptr, (void **)&pAudioClient);
    if (FAILED(hr))
        throw std::runtime_error("Failed to activate IAudioClient from device: " + hresultToString(hr));

    hr = pAudioClient->GetMixFormat(&pwfx);
    if (FAILED(hr))
        throw std::runtime_error("Failed to retrieve mix format from audio client: " + hresultToString(hr));

    // 이벤트 생성
    hEvent = CreateEvent(nullptr, FALSE, FALSE, nullptr);
    if (!hEvent) 
    {
        throw std::runtime_error("Failed to create event: " + win32ErrorToString(GetLastError()));
    }

    // 오디오 클라이언트 초기화 (이벤트 기반)
    hr = pAudioClient->Initialize(AUDCLNT_SHAREMODE_SHARED,
                             AUDCLNT_STREAMFLAGS_LOOPBACK | AUDCLNT_STREAMFLAGS_EVENTCALLBACK,
                             0, 0, pwfx, nullptr);
    if (FAILED(hr))
        throw std::runtime_error("Failed to initialize audio client: " + hresultToString(hr));

    hr = pAudioClient->SetEventHandle(hEvent);
    if (FAILED(hr))
        throw std::runtime_error("Failed to set event handler: " + hresultToString(hr));
}

void AudioCapturer::start() 
{
    HRESULT hr;

    if (capturingEnabled) 
        throw std::runtime_error("AudioCapturer::start() called while already capturing");

    hr = pAudioClient->GetService(__uuidof(IAudioCaptureClient), (void **)&pCaptureClient);
    if (FAILED(hr))
        throw std::runtime_error("Failed to get IAudioCaptureClient: " + hresultToString(hr));

    hr = pAudioClient->Start();
    if (FAILED(hr))
        throw std::runtime_error("Failed to start audio client: " + hresultToString(hr));

    // Start capture thread
    try 
    {
        capturingEnabled = true;
        captureThread = std::make_shared<std::thread>([this]()
            { this->audioCaptureThread(this->hEvent, this->pCaptureClient, this->pAudioClient, this->pwfx); });
    } 
    catch(const std::exception& e) 
    {
        capturingEnabled = false;
        pAudioClient->Stop();
        throw std::runtime_error("Failed to start capture thread: " + e.what());
    }
}
