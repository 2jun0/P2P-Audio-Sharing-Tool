import CoreAudio
import Foundation

final class AudioLoopback {
    private let deviceID: AudioObjectID
    private var ioProcID: AudioDeviceIOProcID?
    private var isRunning = false

    init(_ deviceID: AudioObjectID) {
        self.deviceID = deviceID
    }

    deinit {
        stop()
    }

    func start() throws {
        guard !isRunning else { return }

        var procID: AudioDeviceIOProcID?
        let status = AudioDeviceCreateIOProcID(deviceID, loopbackIOProc, Unmanaged.passUnretained(self).toOpaque(), &procID)
        try status.check("AudioDeviceCreateIOProcID")

        guard let createdID = procID else {
            throw LoopbackError.creation("IOProcID was not created")
        }

        let startStatus = AudioDeviceStart(deviceID, createdID)
        if startStatus != noErr {
            AudioDeviceDestroyIOProcID(deviceID, createdID)
            try startStatus.check("AudioDeviceStart")
        }

        ioProcID = createdID
        isRunning = true
    }

    func stop() {
        guard let procID = ioProcID else { return }

        AudioDeviceStop(deviceID, procID)
        AudioDeviceDestroyIOProcID(deviceID, procID)
        ioProcID = nil
        isRunning = false
    }

    fileprivate func handleIOProc(
        inputData: UnsafePointer<AudioBufferList>?,
        outputData: UnsafeMutablePointer<AudioBufferList>?
    ) -> OSStatus {
        guard let inputData, let outputData else { return noErr }

        let inputBuffers = UnsafeMutableAudioBufferListPointer(mutating: inputData)
        let outputBuffers = UnsafeMutableAudioBufferListPointer(outputData)
        let count = min(inputBuffers.count, outputBuffers.count)

        guard count > 0 else { return noErr }

        for index in 0..<count {
            let src = inputBuffers[index]
            var dst = outputBuffers[index]
            guard let srcPtr = src.mData, let dstPtr = dst.mData else { continue }

            let bytes = min(src.mDataByteSize, dst.mDataByteSize)
            memcpy(dstPtr, srcPtr, Int(bytes))
            dst.mDataByteSize = bytes
            outputBuffers[index] = dst
        }

        return noErr
    }
}

// MARK: - C 콜백 브리지

private func loopbackIOProc(
    _ inDevice: AudioObjectID,
    _ inNow: UnsafePointer<AudioTimeStamp>?,
    _ inInputData: UnsafePointer<AudioBufferList>?,
    _ inInputTime: UnsafePointer<AudioTimeStamp>?,
    _ outOutputData: UnsafeMutablePointer<AudioBufferList>?,
    _ outOutputTime: UnsafePointer<AudioTimeStamp>?,
    _ inClientData: UnsafeMutableRawPointer?
) -> OSStatus {
    guard
        let rawPointer = inClientData,
        let loopback = Unmanaged<AudioLoopback>.fromOpaque(rawPointer).takeUnretainedValue() as AudioLoopback?
    else { return noErr }

    return loopback.handleIOProc(inputData: inInputData, outputData: outOutputData)
}

// MARK: - 에러 헬퍼

private enum LoopbackError: Error {
    case operation(String, OSStatus)
    case creation(String)
}

private extension OSStatus {
    func check(_ label: String) throws {
        guard self == noErr else { throw LoopbackError.operation(label, self) }
    }
}