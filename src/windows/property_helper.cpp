#include "property_helper.hpp"
#include <windows.h>

std::string wideToUtf8(const std::wstring &w)
{
    if (w.empty())
        return {};

    int sizeNeeded = WideCharToMultiByte(CP_UTF8, 0, w.c_str(), (int)w.size(), NULL, 0, NULL, NULL);
    std::string s(sizeNeeded, 0);
    WideCharToMultiByte(CP_UTF8, 0, w.c_str(), (int)w.size(), &s[0], sizeNeeded, NULL, NULL);
    return s;
}