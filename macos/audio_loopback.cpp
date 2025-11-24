#include "audio_loopback.hpp"
#include <cstring>

OSStatus AudioLoopback_ioProc(AudioObjectID inDevice,
                              const AudioTimeStamp *,
                              const AudioBufferList *inInputData,
                              const AudioTimeStamp *,
                              AudioBufferList *outOutputData,
                              const AudioTimeStamp *,
                              void *inClientData)
{
    AudioLoopback *self = static_cast<AudioLoopback *>(inClientData);
    return self->handleIOProc(inInputData, outOutputData);
}

AudioLoopback::AudioLoopback(AudioObjectID deviceID)
    : deviceID(deviceID)
{
}

AudioLoopback::~AudioLoopback()
{
    stop();
}

bool AudioLoopback::start()
{
    if (recording)
        return true;

    recording = true;

    AudioDeviceIOProcID proc = nullptr;
    OSStatus status = AudioDeviceCreateIOProcID(deviceID, AudioLoopback_ioProc, this, &proc);
    if (status != noErr)
        return false;
    ioProcID = proc;

    status = AudioDeviceStart(deviceID, ioProcID);
    if (status != noErr)
    {
        AudioDeviceDestroyIOProcID(deviceID, ioProcID);
        ioProcID = nullptr;
        return false;
    }

    return true;
}

void AudioLoopback::stop()
{
    if (!recording)
        return;

    if (ioProcID != nullptr)
    {
        AudioDeviceStop(deviceID, ioProcID);
        AudioDeviceDestroyIOProcID(deviceID, ioProcID);
        ioProcID = nullptr;
    }

    recording = false;
}

OSStatus AudioLoopback::handleIOProc(const AudioBufferList *inInputData, AudioBufferList *outOutputData)
{
    if (inInputData == nullptr || outOutputData == nullptr)
        return kAudioHardwareNoError;

    const UInt32 numberInputBuffers = inInputData->mNumberBuffers > 0 ? inInputData->mNumberBuffers : 0;
    const UInt32 numberOutputBuffers = outOutputData->mNumberBuffers > 0 ? outOutputData->mNumberBuffers : 0;

    for (UInt32 i = 0; i < numberInputBuffers; ++i)
    {
        const AudioBuffer &inBuf = inInputData->mBuffers[i];
        if (i < numberOutputBuffers)
        {
            AudioBuffer &outBuf = outOutputData->mBuffers[i];
            std::memcpy(outBuf.mData, inBuf.mData, inBuf.mDataByteSize);
        }
    }

    return kAudioHardwareNoError;
}
