#ifndef export_hpp
#define export_hpp

#if defined(_WIN32) || defined(_WIN64)
#ifdef AUDIO_LINK_CORE_DLL_EXPORTS
#define AUDIO_API __declspec(dllexport)
#else
#define AUDIO_API __declspec(dllimport)
#endif
#else
#define AUDIO_API
#endif

#endif /* export_hpp */