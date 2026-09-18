#include "g_memory.h"
#include <string>
#include <sstream>
#include <cstring>

void ParseSignature(const char* sig, std::vector<int>& bytes) {
    std::stringstream ss(sig);
    std::string t;
    while (ss >> t) bytes.push_back((t == "??" || t == "?") ? -1 : std::stoul(t, nullptr, 16));
}

uintptr_t FindPattern(const char* mod, const char* sig) {
    HMODULE hMod = GetModuleHandleA(mod);
    if (!hMod) return 0;

    PIMAGE_DOS_HEADER dH = (PIMAGE_DOS_HEADER)hMod;
    PIMAGE_NT_HEADERS ntH = (PIMAGE_NT_HEADERS)((uint8_t*)hMod + dH->e_lfanew);
    DWORD imgSize = ntH->OptionalHeader.SizeOfImage;

    std::vector<int> pat;
    ParseSignature(sig, pat);
    uint8_t* scan = (uint8_t*)hMod;

    for (DWORD i = 0; i < imgSize - pat.size(); ++i) {
        bool found = true;
        for (size_t j = 0; j < pat.size(); ++j) {
            if (pat[j] != -1 && scan[i + j] != pat[j]) { found = false; break; }
        }
        if (found) return (uintptr_t)&scan[i];
    }
    return 0;
}

void* AllocateNearAddress(uintptr_t target, size_t size) {
    SYSTEM_INFO sI;
    GetSystemInfo(&sI);
    uint64_t pSize = sI.dwPageSize;

    uint64_t start = (target & ~(pSize - 1));
    uint64_t minA = max(start - 0x7FFFFF00, (uint64_t)sI.lpMinimumApplicationAddress);
    uint64_t maxA = min(start + 0x7FFFFF00, (uint64_t)sI.lpMaximumApplicationAddress);

    for (uint64_t a = start; a > minA; a -= pSize) {
        if (void* alloc = VirtualAlloc((void*)a, size, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE)) return alloc;
    }
    for (uint64_t a = start; a < maxA; a += pSize) {
        if (void* alloc = VirtualAlloc((void*)a, size, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE)) return alloc;
    }
    return nullptr;
}

uintptr_t FindString(const char* moduleName, const char* str) {
    HMODULE hMod = GetModuleHandleA(moduleName);
    if (!hMod || !str) return 0;

    PIMAGE_DOS_HEADER dH = (PIMAGE_DOS_HEADER)hMod;
    PIMAGE_NT_HEADERS ntH = (PIMAGE_NT_HEADERS)((uint8_t*)hMod + dH->e_lfanew);
    DWORD imgSize = ntH->OptionalHeader.SizeOfImage;

    size_t strLen = strlen(str) + 1;
    uint8_t* scan = (uint8_t*)hMod;

    for (DWORD i = 0; i < imgSize - strLen; ++i) {
        if (memcmp(&scan[i], str, strLen) == 0) {
            return (uintptr_t)&scan[i];
        }
    }
    return 0;
}

uintptr_t FindRipRef(const char* moduleName, uintptr_t targetAddr, uint8_t regOpcode) {
    HMODULE hMod = GetModuleHandleA(moduleName);
    if (!hMod || !targetAddr) return 0;

    PIMAGE_DOS_HEADER dH = (PIMAGE_DOS_HEADER)hMod;
    PIMAGE_NT_HEADERS ntH = (PIMAGE_NT_HEADERS)((uint8_t*)hMod + dH->e_lfanew);
    DWORD imgSize = ntH->OptionalHeader.SizeOfImage;

    uint8_t* scan = (uint8_t*)hMod;

    for (DWORD i = 0; i < imgSize - 7; ++i) {
        // lea reg, [rip+disp]
        if (scan[i] == 0x48 && scan[i + 1] == 0x8D && scan[i + 2] == regOpcode) {
            int32_t disp = *reinterpret_cast<int32_t*>(&scan[i + 3]);
            uintptr_t ripAfter = (uintptr_t)&scan[i + 7];
            if ((ripAfter + disp) == targetAddr) {
                return (uintptr_t)&scan[i];
            }
        }
    }
    return 0;
}

uintptr_t FindRipRefInRange(uintptr_t targetAddr, uintptr_t rangeStart, uintptr_t rangeEnd, uint8_t regOpcode) {
    if (!targetAddr || !rangeStart || rangeEnd <= rangeStart) return 0;

    uint8_t* scan = reinterpret_cast<uint8_t*>(rangeStart);
    size_t scanLen = rangeEnd - rangeStart;

    for (size_t i = 0; i < scanLen - 7; ++i) {
        if (scan[i] == 0x48 && scan[i + 1] == 0x8D && scan[i + 2] == regOpcode) {
            int32_t disp = *reinterpret_cast<int32_t*>(&scan[i + 3]);
            uintptr_t ripAfter = (uintptr_t)&scan[i + 7];
            if ((ripAfter + disp) == targetAddr) {
                return (uintptr_t)&scan[i];
            }
        }
    }
    return 0;
}

uintptr_t ResolveCallTarget(uintptr_t callInstructionAddr) {
    if (!callInstructionAddr) return 0;

    const uint8_t* p = reinterpret_cast<const uint8_t*>(callInstructionAddr);
    if (p[0] == 0xE8) {
        int32_t rel = *reinterpret_cast<const int32_t*>(p + 1);
        return callInstructionAddr + 5 + rel;
    }
    return 0;
}

uintptr_t FindNthCallForward(uintptr_t startAddr, size_t maxScanBytes, size_t targetCallIndex) {
    if (!startAddr) return 0;

    const uint8_t* p = reinterpret_cast<const uint8_t*>(startAddr);
    size_t callCount = 0;

    for (size_t i = 0; i < maxScanBytes - 5; ++i) {
        if (p[i] == 0xE8) {
            if (callCount == targetCallIndex) {
                return ResolveCallTarget(startAddr + i);
            }
            callCount++;
        }
    }
    return 0;
}

uintptr_t ResolveHUDCallbackByString(const char* moduleName, const char* hudName) {
    uintptr_t pStr = FindString(moduleName, hudName);
    if (!pStr) return 0;

    HMODULE hMod = GetModuleHandleA(moduleName);
    if (!hMod) hMod = GetModuleHandleA(nullptr);
    if (!hMod) return 0;

    PIMAGE_DOS_HEADER dH = (PIMAGE_DOS_HEADER)hMod;
    PIMAGE_NT_HEADERS ntH = (PIMAGE_NT_HEADERS)((uint8_t*)hMod + dH->e_lfanew);
    DWORD imgSize = ntH->OptionalHeader.SizeOfImage;
    uint8_t* scan = (uint8_t*)hMod;

    for (DWORD i = 35; i < imgSize - 35; ++i) {
        if (scan[i] == 0x4C && scan[i + 1] == 0x8D && (scan[i + 2] & 0xC7) == 0x05) {
            int32_t disp = *reinterpret_cast<int32_t*>(&scan[i + 3]);
            uintptr_t ripAfter = (uintptr_t)&scan[i + 7];
            if ((ripAfter + disp) == pStr) {
                DWORD startSearch = (i >= 35) ? (i - 35) : 0;
                DWORD endSearch = (i + 35 < imgSize - 7) ? (i + 35) : (imgSize - 7);

                for (DWORD j = startSearch; j < endSearch; ++j) {
                    if (j >= i && j < i + 7) continue; 
                    if (scan[j] == 0x4C && scan[j + 1] == 0x8D && scan[j + 2] == 0x0D) {
                        int32_t dispCb = *reinterpret_cast<int32_t*>(&scan[j + 3]);
                        uintptr_t ripCbAfter = (uintptr_t)&scan[j + 7];
                        uintptr_t targetCb = ripCbAfter + dispCb;
                        return targetCb;
                    }
                }
            }
        }
    }
    return 0;
}

