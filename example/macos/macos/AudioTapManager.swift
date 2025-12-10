import CoreAudio

public enum TapMute: Int, CaseIterable {
    case unmuted = 0
    case muted = 1
    case mutedWhenTapped = 2
}

public enum TapMixdown: Int, CaseIterable {
    case mono = 0
    case stereo = 1
    case deviceFormat = 2
}

struct TapConfig: Hashable {
    var name: String = "Sample audio tap"
    var processes = Set<AudioObjectID>()
    var isPrivate = false
    var mute: TapMute = .unmuted
    var mixdown = TapMixdown.stereo
    var exclusive = true
    var device: String?
    var streamIndex: UInt? = 0
}

class AudioTapManager {
    func createTap(_ tapConfig: TapConfig) -> AudioObjectID {
        // Create a tap description.
        let description = CATapDescription()

        description.name = tapConfig.name
        description.processes = Array(tapConfig.processes)
        description.isPrivate = tapConfig.isPrivate
        description.muteBehavior =
            CATapMuteBehavior(rawValue: tapConfig.mute.rawValue) ?? description.muteBehavior
        description.isMixdown = tapConfig.mixdown == .mono || tapConfig.mixdown == .stereo
        description.isMono = tapConfig.mixdown == .mono
        description.isExclusive = tapConfig.exclusive
        description.stream = tapConfig.streamIndex

        // Ask the HAL to create a new tap and put the resulting `AudioObjectID` in `tapID`.
        var tapID = AudioObjectID(kAudioObjectUnknown)
        AudioHardwareCreateProcessTap(description, &tapID)
        
        return tapID
    }

    func findTapByName(_ name: String) -> AudioObjectID? {
        var tapListAddress = getPropertyAddress(selector: kAudioHardwarePropertyTapList)
        var propertySize: UInt32 = 0
        AudioObjectGetPropertyDataSize(
            AudioObjectID(kAudioObjectSystemObject), &tapListAddress, 0, nil, &propertySize)

        let tapCount = Int(propertySize) / MemoryLayout<AudioObjectID>.stride
        var list: [AudioObjectID] = [AudioObjectID](repeating: 0, count: tapCount)
        AudioObjectGetPropertyData(
            AudioObjectID(kAudioObjectSystemObject), &tapListAddress, 0, nil, &propertySize,
            &list)

        for id in list {
            var tapPropertyAddress = getPropertyAddress(selector: kAudioTapPropertyDescription)
            var tapPropertySize = UInt32(MemoryLayout<CATapDescription>.stride)
            var tapDescription = CATapDescription()
            _ = withUnsafeMutablePointer(to: &tapDescription) { tapDescription in
                AudioObjectGetPropertyData(
                    id, &tapPropertyAddress, 0, nil, &tapPropertySize, tapDescription)
            }

            if tapDescription.name == name { return id }
        }
        return nil
    }

    func updateTap(_ tapID: AudioObjectID, _ tapConfig: TapConfig) {
        // Get the description of the audio tap.
        var propertyAddress = getPropertyAddress(selector: kAudioTapPropertyDescription)
        var propertySize = UInt32(MemoryLayout<CATapDescription>.stride)
        var description: CATapDescription = CATapDescription()
        _ = withUnsafeMutablePointer(to: &description) { description in
            AudioObjectGetPropertyData(tapID, &propertyAddress, 0, nil, &propertySize, description)
        }

        // Fill out the description properties with the tap config.
        description.name = tapConfig.name
        description.processes = Array(tapConfig.processes)
        description.isPrivate = tapConfig.isPrivate
        description.muteBehavior =
            CATapMuteBehavior(rawValue: tapConfig.mute.rawValue) ?? description.muteBehavior
        description.isMixdown = tapConfig.mixdown == .mono || tapConfig.mixdown == .stereo
        description.isMono = tapConfig.mixdown == .mono
        description.isExclusive = tapConfig.exclusive
        description.deviceUID = tapConfig.device
        description.stream = tapConfig.streamIndex

        _ = withUnsafeMutablePointer(to: &description) { description in
            AudioObjectSetPropertyData(
                tapID, &propertyAddress, 0, nil, propertySize, description)
        }
    }

    func destroyTap(_ tapID: AudioObjectID) {
        AudioHardwareDestroyProcessTap(tapID)
    }
}
