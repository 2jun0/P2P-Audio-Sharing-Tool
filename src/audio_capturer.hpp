
#ifndef audio_capturer_hpp
#define audio_capturer_hpp

#include <windows.h>
#include <mmdeviceapi.h>
#include <audioclient.h>
#include "error_util.hpp"

class AudioCapturer 
{
public:
    std::function<void(uint8_t*, uint32_t)> onReadAudioBuffer;

    AudioCapturer();
    ~AudioCapturer();

    void start();
    void stop();

private:
    std::atomic<bool> capturingEnabled{false};
    std::shared_ptr<std::thread> captureThread;

    // Windows audio capture variables
    IMMDeviceEnumerator *pEnumerator = nullptr;
    IMMDevice *pDevice = nullptr;
    IAudioClient *pAudioClient = nullptr;
    IAudioCaptureClient *pCaptureClient = nullptr;
    WAVEFORMATEX *pwfx = nullptr;
    HANDLE hEvent = nullptr;

    void initAudioDevice();
    void audioCaptureThread(HANDLE hEvent, IAudioCaptureClient *pCaptureClient, IAudioClient *pAudioClient, WAVEFORMATEX *pwfx);
};

#endif /* audio_capturer_hpp */