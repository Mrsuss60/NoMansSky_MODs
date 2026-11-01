#include <windows.h>
#include <iostream>
#include <cstdio>
#include <string>
#include <thread>
#include <chrono>
#include "config_file.h"
#include "camera_mod.h"
#include "camera_ui.h"
#include "globals.h"
#include "logger.h"
#include "camera_mod_veh.h"

void SetModEnabled(bool enable) {
    Config::bModActive.store(enable);

    SetOnFootHooksEnabled(enable);
    VehicleCamera::SetEnabled(enable);

    if (enable) {
        ForceUpdateCamera();
    }

    std::cout << "camera: hooks toggled -> " << (enable ? "enabled" : "disabled") << "\n";
}

void ToggleMod() {
    SetModEnabled(!Config::bModActive.load());
}

void KeyPollLoop() {
    static bool last[256] = { false };
    int configCheckCounter = 0;
    while (Config::bInitialized.load()) {
        if (++configCheckCounter >= 100) { // ~1s poll interval
            configCheckCounter = 0;
            if (HasConfigChanged()) {
                LoadConfig();
            }
        }

        bool save = false;

        auto HandleKey = [&](int key, std::atomic<float>& val, float mod) {
            bool current = (GetAsyncKeyState(key) & 0x8000) != 0;
            if (current) {
                float n = val.load() + mod;
                if (n > 50.0f) n = 50.0f;
                if (n < -50.0f) n = -50.0f;
                val.store(n);
            }
            else if (last[key]) {
                save = true;
            }
            last[key] = current;
        };

        if (Config::EnableHotkeys.load()) {
            bool toggle = (GetAsyncKeyState(Config::ToggleKey) & 0x8000) != 0;
            if (toggle && !last[Config::ToggleKey]) ToggleMod();
            last[Config::ToggleKey] = toggle;

            if (Config::bModActive.load()) {
                HandleKey(Config::IncDistKey, Config::CustomDist, Config::Step);
                HandleKey(Config::DecDistKey, Config::CustomDist, -Config::Step);
                HandleKey(Config::IncWidthKey, Config::CustomX, Config::Step);
                HandleKey(Config::DecWidthKey, Config::CustomX, -Config::Step);
                HandleKey(Config::IncHeightKey, Config::CustomHeight, Config::Step);
                HandleKey(Config::DecHeightKey, Config::CustomHeight, -Config::Step);

                if (save) SaveConfig();
            }
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }
}

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

    if (AllocConsole()) {
        FILE* fp;
        freopen_s(&fp, "NUL", "w", stdout);
        freopen_s(&fp, "NUL", "w", stderr);
        freopen_s(&fp, "NUL", "r", stdin);

        std::ios::sync_with_stdio(true);
        std::cout.clear();
        std::cerr.clear();
        std::cin.clear();

        SetConsoleTitleA("NMS CTPCamera Debug Console");
    }
}

static unsigned int __stdcall ModThread(void*) {
    CreateDebugConsole();
    Logger::Initialize();

    if (IsMicrosoftStoreVersion()) {
        std::cout << "init: microsoft store version detected, not supported for now\n";
        return 0;
    }

    std::cout << "init: loading configuration\n";
    LoadConfig();

    std::cout << "hooks: patching camera bytecode\n";
    ApplyHooks();

    if (!VehicleCamera::ApplyHooks()) {
        std::cout << "vehicle: hooks failed to apply\n";
    }

    std::cout << "ui: hooking camera menu\n";
    if (!CameraUI::Initialize()) {
        std::cout << "ui: failed to hook camera menu\n";
    }

    Config::bInitialized.store(true);
    std::cout << "main: entering key poll loop\n\n";
    KeyPollLoop();

    return 0;
}

BOOL APIENTRY DllMain(HMODULE hModule, DWORD reason, LPVOID) {
    if (reason == DLL_PROCESS_ATTACH) {
        DisableThreadLibraryCalls(hModule);
        CreateThread(nullptr, 0, (LPTHREAD_START_ROUTINE)ModThread, nullptr, 0, nullptr);
    }
    else if (reason == DLL_PROCESS_DETACH) {
        Config::bInitialized.store(false);
        Logger::Shutdown();
    }
    return TRUE;
}