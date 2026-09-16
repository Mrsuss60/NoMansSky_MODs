#pragma once
#include <windows.h>
#include <cstdint>
#include <atomic>

namespace Config {
    inline std::atomic<bool> bInitialized{ false };
    inline bool EnableConsole = false;
    inline std::atomic<bool> EnableHotkeys{ true };

    inline uintptr_t* pGlobalAppPtr = nullptr;

    inline int ToggleHUDKey = VK_F1;

    // Pattern for global app wrapper
    inline const char* SigGlobalAppWrapper = "48 8B 0D ?? ?? ?? ?? F3 0F 10 0D ?? ?? ?? ?? 48 81 C1 ?? ?? ?? ?? E9";

    // Direct ShipHUD Callback Signature
    inline const char* SigShipHUD = "40 53 48 83 EC 20 48 8B D9 48 8B 89 ?? ?? ?? ?? 48 85 C9 74 ?? 48 8B 01 FF 50";

    // Memory Offsets from (*pGlobalAppPtr)
    // Working settings struct = 3205328 (0x30E8D0) + 28000 (0x6D60) = 3233328 (0x315630)
    inline uint32_t OffsetWorkingHUD = 0x315630;
    // Saved settings struct = 3177296 (0x307B50) + 28000 (0x6D60) = 3205296 (0x30E8B0)
    inline uint32_t OffsetSavedHUD   = 0x30E8B0;
}

void ToggleHUDState();
bool GetHUDState(bool& outState);
