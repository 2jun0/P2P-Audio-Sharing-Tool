#ifndef audio_deivce_hpp
#define audio_deivce_hpp

#include <string>

struct AudioDevice
{
    std::string name;
    std::string uid;
    bool hasInput;
    bool hasOutput;
#if defined(__APPLE__)
    int id;
#endif
};

#endif /* audio_deivce_hpp */