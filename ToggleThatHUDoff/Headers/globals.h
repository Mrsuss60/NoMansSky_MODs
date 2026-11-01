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

    // PlayerHUD Indicator Panel Pattern (function immediately following PlayerHUD callback thunk)
    inline const char* SigPlayerHUDIndicator = "48 89 5C 24 ?? 48 89 6C 24 ?? 48 89 74 24 ?? 48 89 7C 24 ?? 41 55 41 56 41 57 48 83 EC 30 8B 69 6C";

    // In 7.11+: HUDHidden (1 = hidden, 0 = visible)
    // In 7.03:  ShowHUD   (0 = hidden, 1 = visible)
    inline bool bHUDHiddenMode = true;

    // Memory Offsets from (*pGlobalAppPtr)
    // 7.11 defaults:
    // Working settings struct = 3205528 (0x30E998) + 28008 (0x6D68) = 3233536 (0x315700)
    inline uint32_t OffsetWorkingHUD = 0x315700;
    // Saved settings struct = 3177480 (0x307C08) + 28008 (0x6D68) = 3205488 (0x30E970)
    inline uint32_t OffsetSavedHUD   = 0x30E970;
}

void ToggleHUDState();
bool GetHUDState(bool& outState);
