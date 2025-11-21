import CoreAudio

@objcMembers
public class LoopbackHelper: NSObject {
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

    /// Ensure a loopback aggregate device with the given uid and name exists.
    // Returns the created/found AudioObjectID or nil on failure.
    @objc public static func ensureLoopbackDeviceWithUid(_ uid: String, name: String)
        -> NSNumber?
    {
        guard let tapID = ensureTap(name) else { return nil }

        let deviceManager = AggregateDeviceManager()
        let deviceConfig = AggregateDeviceConfig(uid: uid, name: name, taps: [tapID])
        var deviceID = deviceManager.findAggregateDeviceByUID(uid)
        if deviceID == nil {
            deviceID = deviceManager.createAggregateDevice(deviceConfig)
        } else {
            deviceManager.updateAggregateDevice(deviceID!, deviceConfig)
        }

        if deviceID == AudioObjectID(kAudioObjectUnknown) {
            return nil
        }

        return NSNumber(value: UInt32(deviceID!))
    }
}
