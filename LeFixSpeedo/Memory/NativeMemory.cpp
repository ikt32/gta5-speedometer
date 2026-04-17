#include "NativeMemory.hpp"
#include "../Util/Logger.hpp"
#include "Versions.h"

#include <inc/main.h>

#include <Windows.h>
#include <Psapi.h>
#include <sstream>

namespace {
template<typename Out>
void split(const std::string& s, char delim, Out result) {
    std::stringstream ss;
    ss.str(s);
    std::string item;
    while (std::getline(ss, item, delim)) {
        *(result++) = item;
    }
}

std::vector<std::string> split(const std::string& s, char delim) {
    std::vector<std::string> elems;
    ::split(s, delim, std::back_inserter(elems));
    return elems;
}

template <typename T>
T CheckAddr(HMODULE lib, const std::string& funcName) {
    FARPROC func = GetProcAddress(lib, funcName.c_str());
    if (!func) {
        LOG(Error, "[Memory] Couldn't get function [{}]", funcName);
        return nullptr;
    }
    LOG(Debug, "[Memory] Found function [{}]", funcName);
    return reinterpret_cast<T>(func);
}

HMODULE nScriptHookVModule = nullptr;
uint8_t* (*nSHVGetScriptHandleBaseAddress)(int handle) = nullptr;
bool nAddressModeLogged = false;
}

namespace mem {
typedef uintptr_t(__fastcall* GetAddressOfEntity_t)(int handle);
GetAddressOfEntity_t gFuncGetAddressOfEntity = nullptr;

void InitShvGetScriptHandleBaseAddress() {
    // getScriptHandleBaseAddress is not present in FiveM
    // Resolve it dynamically so it doesn't fail LoadLibrary in FiveM

    LOG(Debug, "[Memory] Initializing ScriptHookV getScriptHandleBaseAddress");

    nScriptHookVModule = GetModuleHandle("ScriptHookV.dll");
    if (!nScriptHookVModule) {
        LOG(Warning, "[Memory] ScriptHookV.dll not found");
        return;
    }

    nSHVGetScriptHandleBaseAddress = CheckAddr<uint8_t * (*)(int)>(nScriptHookVModule, "?getScriptHandleBaseAddress@@YAPEAEH@Z");
}

void InitGetEntityAddress() {
    auto addr = uintptr_t(0);
    if (!Versions::IsEnhanced()) {
        if (getGameVersion() >= Versions::EGameVersion::L_1_0_3788_0) {
            addr = FindPattern("\x85\xED\x74\x0F\x8B\xCD\xE8\x00\x00\x00\x00\x48\x8B\xF8\x48\x85\xC0\x74\x2E", "xxxxxxx????xxxxxxxx");
            gFuncGetAddressOfEntity = addr ? reinterpret_cast<uintptr_t(*)(int)>(addr + 11 + *(int*)(addr + 7)) : nullptr;
        }
        else {
            addr = FindPattern("\x83\xF9\xFF\x74\x31\x4C\x8B\x0D\x00\x00\x00\x00\x44\x8B\xC1\x49\x8B\x41\x08",
                               "xxxxxxxx????xxxxxxx");
            gFuncGetAddressOfEntity = addr ? reinterpret_cast<uintptr_t(*)(int)>(addr) : nullptr;
        }
    }
    else {
        if (getGameVersion() >= Versions::EGameVersion::E_1_0_1013_33) {
            addr = FindPattern("41 8b 4c 1c ? e8");
            gFuncGetAddressOfEntity = addr ? reinterpret_cast<uintptr_t(*)(int)>(addr + 10 + *(int*)(addr + 6)) : nullptr;
        }
        else {
            addr = FindPattern("83 F9 FF 74 64 41 89 C8");
            gFuncGetAddressOfEntity = addr ? reinterpret_cast<uintptr_t(*)(int)>(addr) : nullptr;
        }
    }
    if (!addr) {
        LOG(Error, "[Memory] Couldn't find GetAddressOfEntity");
        gFuncGetAddressOfEntity = nullptr;
    }
    else {
        LOG(Debug, "[Memory] Found GetAddressOfEntity at 0x{:X}", addr);
    }
}

void init() {
    InitGetEntityAddress();
    InitShvGetScriptHandleBaseAddress();
}

uintptr_t FindPattern(const char* pattern, const char* mask) {
    MODULEINFO modInfo = { nullptr };
    GetModuleInformation(GetCurrentProcess(), GetModuleHandle(nullptr), &modInfo, sizeof(MODULEINFO));

    const char* start_offset = reinterpret_cast<const char*>(modInfo.lpBaseOfDll);
    const uintptr_t size = static_cast<uintptr_t>(modInfo.SizeOfImage);

    intptr_t pos = 0;
    const uintptr_t searchLen = static_cast<uintptr_t>(strlen(mask) - 1);

    for (const char* retAddress = start_offset; retAddress < start_offset + size; retAddress++) {
        if (*retAddress == pattern[pos] || mask[pos] == '?') {
            if (mask[pos + 1] == '\0')
                return (reinterpret_cast<uintptr_t>(retAddress) - searchLen);
            pos++;
        }
        else {
            pos = 0;
        }
    }
    return 0;
}

std::vector<uintptr_t> FindPatterns(const char* pattern, const char* mask) {
    std::vector <uintptr_t> addresses;

    MODULEINFO modInfo = { nullptr };
    GetModuleInformation(GetCurrentProcess(), GetModuleHandle(nullptr), &modInfo, sizeof(MODULEINFO));

    const char* start_offset = reinterpret_cast<const char*>(modInfo.lpBaseOfDll);
    const uintptr_t size = static_cast<uintptr_t>(modInfo.SizeOfImage);

    intptr_t pos = 0;
    const uintptr_t searchLen = static_cast<uintptr_t>(strlen(mask) - 1);

    for (const char* retAddress = start_offset; retAddress < start_offset + size; retAddress++) {
        if (*retAddress == pattern[pos] || mask[pos] == '?') {
            if (mask[pos + 1] == '\0')
                addresses.push_back(reinterpret_cast<uintptr_t>(retAddress) - searchLen);
            pos++;
        }
        else {
            pos = 0;
        }
    }
    return addresses;
}

uintptr_t GetAddressOfEntity(int entity) {
    if (gFuncGetAddressOfEntity)
        return gFuncGetAddressOfEntity(entity);

    // getScriptHandleBaseAddress(entity) can't be used (in FiveM) due to
    // https://github.com/citizenfx/fivem/issues/3354

    if (nSHVGetScriptHandleBaseAddress) {
        if (!nAddressModeLogged) {
            LOG(Warning, "[Memory] GetAddressOfEntity: Fallback to getScriptHandleBaseAddress");
            nAddressModeLogged = true;
        }
        return reinterpret_cast<uintptr_t>(nSHVGetScriptHandleBaseAddress(entity));
    }
    if (!nAddressModeLogged) {
        LOG(Fatal, "[Memory] GetAddressOfEntity: No method available, returning 0");
        nAddressModeLogged = true;
    }
    return 0;
}

uintptr_t FindPattern(const char* pattStr) {
    std::vector<std::string> bytesStr = split(pattStr, ' ');

    MODULEINFO modInfo{};
    GetModuleInformation(GetCurrentProcess(), GetModuleHandle(nullptr), &modInfo, sizeof(MODULEINFO));

    auto* start_offset = static_cast<uint8_t*>(modInfo.lpBaseOfDll);
    const auto size = static_cast<uintptr_t>(modInfo.SizeOfImage);

    uintptr_t pos = 0;
    const uintptr_t searchLen = bytesStr.size();
    std::vector<uint8_t> bytes;
    // Thanks Zolika for the performance improvement!
    for (const auto& str : bytesStr) {
        if (str == "??" || str == "?") bytes.push_back(0);
        else bytes.push_back(static_cast<uint8_t>(std::strtoul(str.c_str(), nullptr, 16)));
    }

    for (auto* retAddress = start_offset; retAddress < start_offset + size; retAddress++) {
        if (bytesStr[pos] == "??" || bytesStr[pos] == "?" ||
            *retAddress == bytes[pos]) {
            if (pos + 1 == bytesStr.size())
                return (reinterpret_cast<uintptr_t>(retAddress) - searchLen + 1);
            pos++;
        }
        else {
            pos = 0;
        }
    }
    return 0;
}
}
