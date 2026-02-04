import Foundation

// MARK: - Make Loopback Audio Device
let audioDeviceName = "Loopback Audio"

func prepareLoopback() {
    let done = DispatchSemaphore(value: 0)

    LoopbackHelper.ensureMicrophonePermission { granted in
        if !granted {
            print("Microphone permission not granted.")
            print("result = false")
        } else {
            let rs = LoopbackHelper.ensureMacosLoopbackDevice(audioDeviceName)
            print("result = \(rs)")
        }
        done.signal()
    }

    done.wait()
}

func removeLoopback() {
    let rs = LoopbackHelper.destroyMacosLoopbackDevice(audioDeviceName)
    print("removed = \(rs)")
}

// MARK: - Make Random peerId
func makePeerId() -> String {
    let alphabet = Array("0123456789abcdef")
    return String((0..<6).map { _ in alphabet.randomElement()! })
}

// MARK: - Main entry
func main() {
    prepareLoopback()

    let args = CommandLine.arguments
    var port = 6000
    var peerId = ""

    if args.count > 1 {
        if let p = Int(args[1]) {
            port = p
        } else {
            print("Invalid port argument: \(args[1])")
            return
        }
    }

    peerId = (args.count > 2) ? args[2] : makePeerId()

    if args.count <= 1 {
        print("Usage: \(args[0]) [port] [peerId]")
        print("  port: local port for session discovery/audio (default: 6000)")
        print("  peerId: 6-char hex id (default: random)")
        print("")
    }

    let streamer = AudioStreamerWrapper(
        port: Int32(port), myId: peerId, myName: peerId, myType: "MacOS")
    streamer.start()

    let audioDeviceManager = streamer.getAudioDeviceManager()
    let devices = audioDeviceManager.findAllAudioDevices()
    let loopbackDevice = devices.first { device in device.name == audioDeviceName }

    // Interactive console loop
    print("\n=== Commands ===")
    print("send <peerId>")
    print("recv <peerId>")
    print("stop-send <peerId>")
    print("stop-recv <peerId>")
    print("list")
    print("quit\n")

    while true {
        guard let line = readLine() else { continue }
        let parts = line.split(separator: " ").map { String($0) }

        if parts.count == 0 { continue }

        let cmd = parts[0]
        let target = parts.count >= 2 ? parts[1] : ""

        if cmd == "quit" { break }

        switch cmd {
        case "send":
            streamer.startSending(to: target, inputDeviceUID: loopbackDevice?.uid)
        case "recv":
            streamer.startReceiving(from: target, outputDeviceUID: nil)
        case "stop-send":
            streamer.stopSending(to: target)
        case "stop-recv":
            streamer.stopReceiving(from: target)
        case "list":
            let peers = streamer.getPeers()
            if peers.isEmpty {
                print("No peers found.")
            } else {
                print("Known peers:")
                for peer in peers {
                    print(" - \(peer)")
                }
            }
        default:
            print("Unknown command: \(cmd)")
        }
    }

    removeLoopback()
}

// 실행 시작
main()
