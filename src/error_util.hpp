#ifndef error_util_hpp
#define error_util_hpp

#include <string>
#include <Windows.h>

std::string hresultToString(HRESULT hr);
std::string win32ErrorToString(DWORD err);

#endif /* error_util_hpp */