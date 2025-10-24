# This is dummy to learn gstreamer

# Sender
gst-launch-1.0 -v \
audiotestsrc ! audioconvert ! audioresample ! opusenc ! rtpopuspay ! udpsink host=192.168.0.100 port=5002

# Receiver
gst-launch-1.0 -v \
udpsrc port=5002 caps="application/x-rtp, media=(string)audio, clock-rate=(int)48000, encoding-name=(string)OPUS" ! \
rtpopusdepay ! opusdec ! audioconvert ! audioresample ! autoaudiosink
