import CoreAudio
import Foundation

public struct AudioDeviceObjc: Equatable, Hashable {
    public let name: String
    public let uid: String
    public let hasInput: Bool
    public let hasOutput: Bool
    public let deviceID: AudioObjectID

    public init(name: String, uid: String, hasInput: Bool, hasOutput: Bool, deviceID: AudioObjectID)
    {
        self.name = name
        self.uid = uid
        self.hasInput = hasInput
        self.hasOutput = hasOutput
        self.deviceID = deviceID
    }
}
