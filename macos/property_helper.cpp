#include "property_helper.hpp"

AudioObjectPropertyAddress getPropertyAddress(AudioObjectPropertySelector selector, AudioObjectPropertyScope scope, AudioObjectPropertyElement element)
{
    return {selector, scope, element};
}

std::string getStringProperty(AudioObjectID deviceID, AudioObjectPropertySelector selector)
{
    AudioObjectPropertyAddress addr = getPropertyAddress(selector);
    UInt32 size = sizeof(CFStringRef);
    CFStringRef cfStr = nullptr;
    AudioObjectGetPropertyData(deviceID, &addr, 0, nullptr, &size, &cfStr);

    auto len = CFStringGetLength(cfStr);
    auto maxSize = CFStringGetMaximumSizeForEncoding(len, kCFStringEncodingUTF8) + 1;
    std::string str;
    str.resize(maxSize);
    CFStringGetCString(cfStr, str.data(), maxSize, kCFStringEncodingUTF8);
    return str;
}