#ifndef audio_deivce_hpp
#define audio_deivce_hpp

#include <string>

struct AudioDevice
{
    std::string name;
    std::string uid;
    int inputChannels;
    int outputChannels;
#if defined(__APPLE__)
    int id;
#endif
};

#endif /* audio_deivce_hpp */