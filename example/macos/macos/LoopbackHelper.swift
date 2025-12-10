import AVFoundation
import CoreAudio

public class LoopbackHelper: NSObject {
    static var audioLoopback: AudioLoopbackWrapper?

    private static func isMicrophonePermissionGranted() -> Bool {
        return AVCaptureDevice.authorizationStatus(for: .audio) == .authorized
    }

    public static func ensureMicrophonePermission(_ completion: @escaping (Bool) -> Void) {
        let status = AVCaptureDevice.authorizationStatus(for: .audio)
        switch status {
        case .authorized:
            completion(true)
        default:
            AVCaptureDevice.requestAccess(for: .audio) { granted in
                DispatchQueue.main.asyncAfter(deadline: .now() + 0.3) {
                    completion(granted)
                }
            }
        }
    }

    // Ensure a tap exists for the given base name. Returns the created/found AudioObjectID or nil on failure.
    private static func ensureTap(_ name: String) -> AudioObjectID? {
        let tapManager = AudioTapManager()
        let tapConfig = TapConfig(name: "\(name)-tap")
        var tapID = tapManager.findTapByName(tapConfig.name)
        if tapID == nil {
            tapID = tapManager.createTap(tapConfig)
        } else {
            tapManager.updateTap(tapID!, tapConfig)
        }

        if tapID == AudioObjectID(kAudioObjectUnknown) {
            return nil
        }

        return tapID
    }

    // Ensure a loopback aggregate device with the given uid and name exists.
    // Returns the created/found AudioObjectID or nil on failure.
    private static func ensureLoopbackDevice(_ name: String)
        -> AudioDeviceID?
    {
        guard let tapID = ensureTap(name) else { return nil }

        let deviceManager = AggregateDeviceManager()
        let deviceConfig = AggregateDeviceConfig(name: name, taps: [tapID])
        // let deviceConfig = AggregateDeviceConfig(name: name, taps: [])
        var deviceID = deviceManager.findAggregateDeviceByName(name)
        if deviceID == nil {
            deviceID = deviceManager.createAggregateDevice(deviceConfig)
        } else {
            deviceManager.updateAggregateDevice(deviceID!, deviceConfig)
        }

        if deviceID == AudioObjectID(kAudioObjectUnknown) {
            return nil
        }

        return deviceID
    }

    public static func ensureMacosLoopbackDevice(_ name: String) -> Bool {
        guard isMicrophonePermissionGranted() else { return false }

        let deviceID = ensureLoopbackDevice(name)
        guard deviceID != nil else { return false }

        if audioLoopback == nil {
            audioLoopback = AudioLoopbackWrapper(deviceID: deviceID!)
            if !(audioLoopback!.start()) {
                audioLoopback = nil
                return false
            }
        }

        return true
    }

    public static func destroyMacosLoopbackDevice(_ name: String) -> Bool {
        let deviceManager = AggregateDeviceManager()
        let deviceID = deviceManager.findAggregateDeviceByName(name)
        if deviceID != nil {
            deviceManager.destroyAggregateDevice(deviceID!)
        }

        let tapManager = AudioTapManager()
        let tapID = tapManager.findTapByName("\(name)-tap")
        if tapID != nil {
            tapManager.destroyTap(tapID!)
        }
        return true
    }
}
