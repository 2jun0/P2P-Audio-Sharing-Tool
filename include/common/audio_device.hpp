#ifndef audio_device_hpp
#define audio_device_hpp

#include <string>

struct AudioDevice
{
    std::string name;
    std::string uid;
    bool hasInput;
    bool hasOutput;
};

#endif /* audio_device_hpp */
