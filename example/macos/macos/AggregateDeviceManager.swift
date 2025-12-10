import CoreAudio

struct AggregateDeviceConfig: Hashable {
    var name: String = "Sample aggregate device"
    var isPrivate = false
    var taps = Set<AudioObjectID>()
}

class AggregateDeviceManager {
    func createAggregateDevice(_ config: AggregateDeviceConfig) -> AudioObjectID {        
        let description =
            [
                kAudioAggregateDeviceUIDKey: UUID().uuidString,
                kAudioAggregateDeviceNameKey: config.name,
                kAudioAggregateDeviceIsPrivateKey: NSNumber(value: config.isPrivate),
                kAudioAggregateDeviceTapListKey: config.taps.map { tapID in
                    [
                        kAudioSubTapDriftCompensationKey: true,
                        kAudioSubTapUIDKey: getTapUID(tapID),
                    ]
                },
            ] as CFDictionary

        var id: AudioObjectID = AudioObjectID(kAudioObjectUnknown)
        AudioHardwareCreateAggregateDevice(description, &id)

        return id
    }

    func findAggregateDeviceByName(_ name: String) -> AudioObjectID? {
        var deviceListAddress = getPropertyAddress(selector: kAudioHardwarePropertyDevices)
        var propertySize: UInt32 = 0
        AudioObjectGetPropertyDataSize(
            AudioObjectID(kAudioObjectSystemObject), &deviceListAddress, 0, nil, &propertySize)

        let deviceCount = Int(propertySize) / MemoryLayout<AudioObjectID>.stride
        var list: [AudioObjectID] = [AudioObjectID](repeating: 0, count: deviceCount)

        AudioObjectGetPropertyData(
            AudioObjectID(kAudioObjectSystemObject), &deviceListAddress, 0, nil, &propertySize,
            &list)

        for id in list {
            var propertyAddress = getPropertyAddress(selector: kAudioObjectPropertyName)
            var propertySize = UInt32(MemoryLayout<CFString>.stride)
            var deviceName: CFString = "" as CFString
            _ = withUnsafeMutablePointer(to: &deviceName) { deviceName in
                AudioObjectGetPropertyData(id, &propertyAddress, 0, nil, &propertySize, deviceName)
            }

            if deviceName as String == name { return id }
        }
        return nil
    }

    func updateAggregateDevice(_ deviceID: AudioObjectID, _ config: AggregateDeviceConfig) {
        var description =
            [
                kAudioAggregateDeviceNameKey: config.name,
                kAudioAggregateDeviceIsPrivateKey: NSNumber(value: config.isPrivate),
                kAudioAggregateDeviceTapListKey: Array(config.taps),
            ] as CFDictionary

        var propertyAddress = getPropertyAddress(selector: kAudioAggregateDevicePropertyComposition)
        let propertySize = UInt32(MemoryLayout<CFDictionary>.stride)
        _ = withUnsafeMutablePointer(to: &description) { description in
            AudioObjectSetPropertyData(
                deviceID, &propertyAddress, 0, nil, propertySize, description)
        }
    }

    func destroyAggregateDevice(_ deviceID: AudioObjectID) {
        AudioHardwareDestroyAggregateDevice(deviceID)
    }

    private func getTapUID(_ tapID: AudioObjectID) -> CFString {
        // Get the UID of the audio tap.
        var propertyAddress = getPropertyAddress(selector: kAudioTapPropertyUID)
        var propertySize = UInt32(MemoryLayout<CFString>.stride)
        var tapUID: CFString = "" as CFString
        _ = withUnsafeMutablePointer(to: &tapUID) { tapUID in
            AudioObjectGetPropertyData(tapID, &propertyAddress, 0, nil, &propertySize, tapUID)
        }
        return tapUID
    }
}
