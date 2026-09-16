#include <windows.h>
#include <iostream>
#include <iomanip>
#include <cstdio>
#include "globals.h"
#include "g_memory.h"
#include "logger.h"
#include "config_file.h"

static bool IsMicrosoftStoreVersion() {
    char exePath[MAX_PATH];
    if (!GetModuleFileNameA(nullptr, exePath, MAX_PATH)) return false;
    std::string dir = exePath;
    size_t pos = dir.find_last_of("\\/");
    if (pos != std::string::npos) dir = dir.substr(0, pos + 1);

    const char* msDlls[] = {
        "PartyXboxLive.dll",
        "PlayFabCore.GDK.dll",
        "PlayFabMultiplayerGDK.dll",
        "PlayFabServices.GDK.dll"
    };

    for (const char* dll : msDlls) {
        std::string fullPath = dir + dll;
        DWORD attr = GetFileAttributesA(fullPath.c_str());
        if (attr != INVALID_FILE_ATTRIBUTES && !(attr & FILE_ATTRIBUTE_DIRECTORY)) {
            return true;
        }
        if (GetModuleHandleA(dll) != nullptr) {
            return true;
        }
    }
    return false;
}

static void CreateDebugConsole() {
    if (!Config::EnableConsole) return;

    bool hasConsole = AllocConsole();
    if (!hasConsole) {
        hasConsole = (GetConsoleWindow() != nullptr);
    }

    if (hasConsole) {
        FILE* fp;
        freopen_s(&fp, "NUL", "w", stdout);
        freopen_s(&fp, "NUL", "w", stderr);
        freopen_s(&fp, "NUL", "r", stdin);

        std::ios::sync_with_stdio(true);
        std::cout.clear();
        std::cerr.clear();
        std::cin.clear();

        SetConsoleTitleA("ToggleThatHUD Log");
    }
}

static bool IsMemoryReadable(void* ptr, size_t size) {
    if (!ptr) return false;
    __try {
        volatile char* p = (volatile char*)ptr;
        for (size_t i = 0; i < size; ++i) {
            char dummy = p[i];
            (void)dummy;
        }
        return true;
    }
    __except (EXCEPTION_EXECUTE_HANDLER) {
        return false;
    }
}

static uintptr_t GetApplicationBase() {
    if (!Config::pGlobalAppPtr) return 0;
    __try {
        return *Config::pGlobalAppPtr;
    }
    __except (EXCEPTION_EXECUTE_HANDLER) {
        return 0;
    }
}

static bool g_bHUDHidden = false;
static uint8_t* g_pPlayerHUD = nullptr;
static uint8_t* g_pShipHUD   = nullptr;
static uint8_t g_origPlayerHUD[3] = { 0 };
static uint8_t g_origShipHUD[3]   = { 0 };
static const uint8_t g_patchRet[3] = { 0x31, 0xC0, 0xC3 }; // xor eax, eax; ret

static bool PatchMemory(uint8_t* target, const uint8_t* bytes, size_t len) {
    if (!target) return false;
    DWORD oldProtect;
    if (VirtualProtect(target, len, PAGE_EXECUTE_READWRITE, &oldProtect)) {
        memcpy(target, bytes, len);
        VirtualProtect(target, len, oldProtect, &oldProtect);
        FlushInstructionCache(GetCurrentProcess(), target, len);
        return true;
    }
    return false;
}

static bool IsHUDCurrentlyPatched() {
    if (!g_pPlayerHUD) return false;
    return (g_pPlayerHUD[0] == 0x31 && g_pPlayerHUD[1] == 0xC0 && g_pPlayerHUD[2] == 0xC3);
}

bool GetHUDState(bool& outState) {
    outState = !IsHUDCurrentlyPatched();
    return true;
}

void ToggleHUDState() {
    bool currentlyHidden = IsHUDCurrentlyPatched();
    bool shouldHide = !currentlyHidden;

    if (shouldHide) {
        if (g_pPlayerHUD) PatchMemory(g_pPlayerHUD, g_patchRet, 3);
        if (g_pShipHUD)   PatchMemory(g_pShipHUD, g_patchRet, 3);
    } else {
        if (g_pPlayerHUD) PatchMemory(g_pPlayerHUD, g_origPlayerHUD, 3);
        if (g_pShipHUD)   PatchMemory(g_pShipHUD, g_origShipHUD, 3);
    }

    g_bHUDHidden = shouldHide;

    uintptr_t appBase = GetApplicationBase();
    if (appBase) {
        uint8_t* pWorking = reinterpret_cast<uint8_t*>(appBase + Config::OffsetWorkingHUD);
        uint8_t* pSaved   = reinterpret_cast<uint8_t*>(appBase + Config::OffsetSavedHUD);
        uint8_t settingVal = shouldHide ? 0 : 1;
        if (IsMemoryReadable(pWorking, sizeof(uint8_t))) *pWorking = settingVal;
        if (IsMemoryReadable(pSaved, sizeof(uint8_t)))   *pSaved   = settingVal;
    }

    //LOG << "hud: HUD toggled -> " << (shouldHide ? "DISABLED (0)" : "ENABLED (1)") << "\n";
}

static bool ApplyHooks() {
    LOG << "hud: scanning for global app wrapper signature\n";

    uintptr_t sigWrapper = FindPattern("NMS.exe", Config::SigGlobalAppWrapper);
    if (sigWrapper) {
        LOG << "hud: signature match @ 0x" << std::hex << sigWrapper << std::dec << "\n";

        int32_t relApp = *reinterpret_cast<int32_t*>(sigWrapper + 3);
        Config::pGlobalAppPtr = reinterpret_cast<uintptr_t*>(sigWrapper + 7 + relApp);

        LOG << "hud: global app pointer @ 0x" << std::hex << (uintptr_t)Config::pGlobalAppPtr << std::dec << "\n";
    } else {
        LOG << "hud: global app wrapper signature not found\n";
    }

    uintptr_t nmsBase = (uintptr_t)GetModuleHandleA(nullptr);

    // Dynamic resolution of PlayerHUD callback via registration string
    uintptr_t addrPlayerHUD = ResolveHUDCallbackByString("NMS.exe", "Game/HUD/PlayerHUD");
    if (!addrPlayerHUD && nmsBase) {
        addrPlayerHUD = nmsBase + 0x9A5950;
    }

    if (addrPlayerHUD && IsMemoryReadable(reinterpret_cast<void*>(addrPlayerHUD), 3)) {
        uint8_t* p = reinterpret_cast<uint8_t*>(addrPlayerHUD);
        if (*p == 0xE9) {
            g_pPlayerHUD = p;
            memcpy(g_origPlayerHUD, g_pPlayerHUD, 3);
            LOG << "hud: PlayerHUD callback resolved @ 0x" << std::hex << (uintptr_t)g_pPlayerHUD << std::dec << "\n";
        }
    }

    if (!g_pPlayerHUD) {
        LOG << "hud: PlayerHUD callback not found\n";
    }

    // Dynamic resolution of ShipHUD callback via registration string or signature
    uintptr_t addrShipHUD = ResolveHUDCallbackByString("NMS.exe", "Game/HUD/ShipHUD");
    if (!addrShipHUD) {
        addrShipHUD = FindPattern("NMS.exe", Config::SigShipHUD);
    }
    if (!addrShipHUD && nmsBase) {
        addrShipHUD = nmsBase + 0x9D3900;
    }

    if (addrShipHUD && IsMemoryReadable(reinterpret_cast<void*>(addrShipHUD), 3)) {
        uint8_t* p = reinterpret_cast<uint8_t*>(addrShipHUD);
        if (p[0] == 0x40 && p[1] == 0x53) {
            g_pShipHUD = p;
            memcpy(g_origShipHUD, g_pShipHUD, 3);
            LOG << "hud: ShipHUD callback resolved @ 0x" << std::hex << (uintptr_t)g_pShipHUD << std::dec << "\n";
        }
    }

    if (!g_pShipHUD) {
        LOG << "hud: ShipHUD callback not found\n";
    }

    return (Config::pGlobalAppPtr != nullptr) || (g_pPlayerHUD != nullptr);
}

static void KeyPollLoop() {
    bool wasKeyPressed = (GetAsyncKeyState(Config::ToggleHUDKey) & 0x8000) != 0;
    uint64_t lastConfigCheckTime = 0;

    while (Config::bInitialized.load()) {
        uint64_t nowMs = GetTickCount64();
        if (nowMs - lastConfigCheckTime >= 1000) {
            lastConfigCheckTime = nowMs;
            if (HasConfigChanged()) {
                LoadConfig();
                LOG << "init: configuration reloaded\n";
            }
        }

        if (Config::EnableHotkeys.load()) {
            bool isPressed = (GetAsyncKeyState(Config::ToggleHUDKey) & 0x8000) != 0;
            if (isPressed && !wasKeyPressed) {
                ToggleHUDState();
            }
            wasKeyPressed = isPressed;
        }

        Sleep(10);
    }
}

static unsigned int __stdcall ModThread(void*) {
    CreateDebugConsole();
    Logger::Initialize("ToggleThatHUD.log");

    if (IsMicrosoftStoreVersion()) {
        LOG << "init: microsoft store version detected, not supported\n";
        return 0;
    }

    LOG << "init: loading configuration\n";
    LoadConfig();

    LOG << "init: resolving engine pointers\n";
    if (!ApplyHooks()) {
        LOG << "hud: failed to resolve application pointer\n";
        return 1;
    }

    Config::bInitialized.store(true);
    LOG << "main: entering key poll loop\n\n";
    KeyPollLoop();

    return 0;
}

BOOL APIENTRY DllMain(HMODULE hModule, DWORD reason, LPVOID lpReserved) {
    if (reason == DLL_PROCESS_ATTACH) {
        DisableThreadLibraryCalls(hModule);
        CreateThread(nullptr, 0, (LPTHREAD_START_ROUTINE)ModThread, nullptr, 0, nullptr);
    }
    else if (reason == DLL_PROCESS_DETACH) {
        Config::bInitialized.store(false);
        if (g_bHUDHidden) {
            if (g_pPlayerHUD) PatchMemory(g_pPlayerHUD, g_origPlayerHUD, 3);
            if (g_pShipHUD)   PatchMemory(g_pShipHUD, g_origShipHUD, 3);
        }
        Logger::Shutdown();
    }
    return TRUE;
}
