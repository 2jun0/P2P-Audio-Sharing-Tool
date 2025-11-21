import CoreAudio

struct AggregateDeviceConfig: Hashable {
    var uid: String
    var name: String = "Sample aggregate device"
    var isPrivate = false
    var taps = Set<AudioObjectID>()
}

class AggregateDeviceManager {
    func createAggregateDevice(_ config: AggregateDeviceConfig) -> AudioObjectID {
        let description =
            [
                kAudioAggregateDeviceUIDKey: config.uid,
                kAudioAggregateDeviceNameKey: config.name,
                kAudioAggregateDeviceIsPrivateKey: NSNumber(value: config.isPrivate),
                kAudioAggregateDeviceTapListKey: Array(config.taps),
            ] as CFDictionary

        var id: AudioObjectID = AudioObjectID(kAudioObjectUnknown)
        AudioHardwareCreateAggregateDevice(description, &id)

        return id
    }

    func findAggregateDeviceByUID(_ uid: String) -> AudioObjectID? {
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
            var propertyAddress = getPropertyAddress(selector: kAudioDevicePropertyDeviceUID)
            var propertySize = UInt32(MemoryLayout<CFString>.stride)
            var deviceUID: CFString = "" as CFString
            AudioObjectGetPropertyData(id, &propertyAddress, 0, nil, &propertySize, &deviceUID)

            if deviceUID as String == uid { return id }
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
        AudioObjectSetPropertyData(deviceID, &propertyAddress, 0, nil, propertySize, &description)
    }

    func destroyAggregateDevice(_ deviceID: AudioObjectID) {
        AudioHardwareDestroyAggregateDevice(deviceID)
    }
}
