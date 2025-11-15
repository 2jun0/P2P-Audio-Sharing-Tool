# Audio [가명]

## GStreamer Install

### For Windows

- Go to https://gstreamer.freedesktop.org/download/
- Download and run installers
  - Runtime Installer
  - Development Installer
- Add binary file to system environment variable
  - C:\Program Files\gstreamer\1.0\msvc_x86_64\bin
- Set env
  ```bash
  $env:PKG_CONFIG_PATH="C:/Program Files/GStreamer/1.0/msvc_x86_64/lib/pkgconfig"
  ```

### For Macos

```bash
brew install gstreamer
brew install gst-plugins-base gst-plugins-good
```

### Check

```base
gst-launch-1.0 --version
```

## Signaling flow

```mermaid
sequenceDiagram
    participant PeerA as Peer A
    participant PeerB as Peer B

    Note over PeerA,PeerB: Ping to check presence
    PeerA<<-->>PeerB: Ping ("Are you there?")

    Note over PeerA,PeerB: Pong to share intents and states
    PeerA->>PeerB: Pong ("I want to send")
    PeerB->>PeerA: Pong ("I want to receive")

    Note over PeerA,PeerB: Both agree, start audio transmission
    PeerB->>PeerA: Open RTP port (ready to receive)
    PeerA->>PeerB: Start sending audio via RTP
```

## Third-party libraries

This project uses the following third-party libraries:

- [JSON for Modern C++](https://github.com/nlohmann/json) by Niels Lohmann  
  Licensed under the MIT License.
