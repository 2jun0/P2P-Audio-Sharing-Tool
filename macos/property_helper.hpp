#ifndef property_helper_hpp
#define property_helper_hpp

#include <CoreAudio/AudioHardware.h>
#include <string>

AudioObjectPropertyAddress getPropertyAddress(AudioObjectPropertySelector selector, AudioObjectPropertyScope scope = kAudioObjectPropertyScopeGlobal, AudioObjectPropertyElement element = kAudioObjectPropertyElementMain);
std::string getStringProperty(AudioObjectID deviceID, AudioObjectPropertySelector selector);

#endif /* property_helper_hpp */