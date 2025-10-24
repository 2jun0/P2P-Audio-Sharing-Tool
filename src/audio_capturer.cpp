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

void AudioCapturer::stop()
{
    HRESULT hr;

    if (!capturingEnabled)
        return;

    capturingEnabled = false;

    if (captureThread && captureThread->joinable()) 
    {
        captureThread->join();
        captureThread.reset();
    }

    if (pAudioClient)
    {
        hr = pAudioClient->Stop();
        if (FAILED(hr))
            throw std::runtime_error("Failed to stop audio client: " + hresultToString(hr));
    }
}

void AudioCapturer::audioCaptureThread(HANDLE hEvent, IAudioCaptureClient *pCaptureClient, IAudioClient *pAudioClient, WAVEFORMATEX *pwfx)
{
    HRESULT hr;

    if (!onReadAudioBuffer)
    {
        std::cerr << "[AudioCapturer] onReadAudioBuffer is null. Exiting thread." << std::endl;
        return;
    }

    while (capturingEnabled)
    {
        DWORD waitResult = WaitForSingleObject(hEvent, 2000); // 최대 2초 대기

        // 빈 패킷 보내기 (연결 유지용)
        if (waitResult == WAIT_TIMEOUT)
        {
            // Sending empty packet to keep connection alive.
            uint32_t nFrames = 480;
            std::vector<uint8_t> silence(pwfx->nBlockAlign * nFrames, 0);
            onReadAudioBuffer(silence.data(), nFrames);
            continue;
        }

        if (waitResult != WAIT_OBJECT_0)
        {
            std::cerr << "[AudioCapturer] Event wait failed. Code: " << waitResult << std::endl;
            break;
        }

        uint32_t packetLength = 0;
        hr = pCaptureClient->GetNextPacketSize(&packetLength);
        if (FAILED(hr))
        {
            std::cerr << "[AudioCapturer] Failed to get packet size (GetNextPacketSize): " << hresultToString(hr) << std::endl;
            break;
        }

        while (packetLength > 0)
        {
            uint8_t *pData;
            uint32_t nFrames;
            DWORD flags;

            hr = pCaptureClient->GetBuffer(&pData, &nFrames, &flags, nullptr, nullptr);
            if (FAILED(hr))
            {
                std::cerr << "[AudioCapturer] Failed to get buffer (GetBuffer): " << hresultToString(hr) << std::endl;
                break;
            }

            onReadAudioBuffer(pData, nFrames);
            pCaptureClient->ReleaseBuffer(nFrames);
            hr = pCaptureClient->GetNextPacketSize(&packetLength);
            if (FAILED(hr))
            {
                std::cerr << "[AudioCapturer] Failed to get packet size (GetNextPacketSize): " << hresultToString(hr) << std::endl;
                break;
            }
        }
    }

    std::cout << "[AudioCapturer] Capture thread exiting" << std::endl;
}