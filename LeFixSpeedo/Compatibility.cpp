#include "Compatibility.h"
#include "Util/Logger.hpp"
#include <Windows.h>
#include <string>

namespace MT {
    bool        (*NeutralGear           )() = nullptr;
    int         (*GetShiftIndicator     )() = nullptr;

    HMODULE GearsModule = nullptr;
    bool Available;
}

template <typename T>
T CheckAddr(HMODULE lib, const std::string& funcName)
{
    FARPROC func = GetProcAddress(lib, funcName.c_str());
    if (!func)
    {
        LOG(Error, "Couldn't get function [{}]", funcName);
        return nullptr;
    }
    LOG(Info, "Found function [{}]", funcName);
    return reinterpret_cast<T>(func);
}

void setupCompatibility() {
    LOG(Info, "Setting up Manual Transmission compatibility");
    MT::GearsModule = GetModuleHandle("Gears.asi");
    if (!MT::GearsModule) {
        MT::Available = false;
        LOG(Warning, "Gears.asi not found");
        return;
    }
    MT::Available = true;

    MT::NeutralGear = CheckAddr<bool (*)()>(MT::GearsModule, "MT_NeutralGear");
    MT::Available &= MT::NeutralGear != nullptr;

    MT::GetShiftIndicator = CheckAddr<int (*)()>(MT::GearsModule, "MT_GetShiftIndicator");
    MT::Available &= MT::GetShiftIndicator != nullptr;
}

void releaseCompatibility() {
    if (MT::GearsModule) {
        MT::GearsModule = nullptr;
        MT::Available = false;
    }
}
