# This is dummy to learn gstreamer

# Sender
gst-launch-1.0 -v audiotestsrc ! audioconvert ! audioresample ! opusenc ! rtpopuspay ! udpsink host=127.0.0.1 port=61806

# Receiver
gst-launch-1.0 -v udpsrc port=0 caps="application/x-rtp, media=(string)audio, clock-rate=(int)48000, encoding-name=(string)OPUS" ! rtpopusdepay ! opusdec ! audioconvert ! audioresample ! autoaudiosink
