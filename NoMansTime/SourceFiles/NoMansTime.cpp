#include <windows.h>
#include <iostream>
#include <iomanip>
#include <cstdio>
#include <cmath>
#include "globals.h"
#include "g_memory.h"
#include "logger.h"
#include "timeofday_ui.h"
#include "config_file.h"

typedef void* (__fastcall* tSetTimeOfDayDelta)(void* pEnv, float deltaDegrees);
typedef void (__fastcall* tStepTimeForward)();
typedef void (__fastcall* tStepTimeBackward)();

static tSetTimeOfDayDelta fnSetTimeOfDay = nullptr;
static tStepTimeForward   fnStepForward = nullptr;
static tStepTimeBackward  fnStepBackward = nullptr;

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

        SetConsoleTitleA("NoMansTimeLog.log");
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

void* GetEnvironmentInstance() {
    if (!Config::pGlobalAppPtr) return nullptr;

    __try {
        uintptr_t appBase = *Config::pGlobalAppPtr;
        if (!appBase) return nullptr;

        void* pEnv = (void*)(appBase + Config::OffsetEnvFromApp);
        if (IsMemoryReadable(pEnv, 0x610)) {
            return pEnv;
        }
    }
    __except (EXCEPTION_EXECUTE_HANDLER) {
    }
    return nullptr;
}

static void FormatTimeDisplay(float angleDegrees, int& outHours, int& outMinutes) {
    float normalized = angleDegrees / 360.0f;
    while (normalized < 0.0f) normalized += 1.0f;
    while (normalized >= 1.0f) normalized -= 1.0f;

    float totalMinutesFloat = normalized * 24.0f * 60.0f;
    int totalMinutes = static_cast<int>(std::floor(totalMinutesFloat + 0.5f));
    outHours = (totalMinutes / 60) % 24;
    outMinutes = totalMinutes % 60;
}

void AdjustTimeOfDay(float deltaDegrees) {
    void* pEnv = GetEnvironmentInstance();
    if (!pEnv) return;

    __try {
        if (fnSetTimeOfDay) {
            fnSetTimeOfDay(pEnv, deltaDegrees);
        }

        if (Config::EnableConsole) {
            float angleAfter = *(float*)((uintptr_t)pEnv + Config::OffsetTimeAngle);
            int hours = 0, minutes = 0;
            FormatTimeDisplay(angleAfter, hours, minutes);

            std::cout << "time: step "
                      << (deltaDegrees >= 0 ? "+" : "") << std::fixed << std::setprecision(1) << deltaDegrees << " deg -> "
                      << "time " << std::setfill('0') << std::setw(2) << hours << ":"
                      << std::setfill('0') << std::setw(2) << minutes
                      << " (angle " << angleAfter << " deg)\n";
        }
    }
    __except (EXCEPTION_EXECUTE_HANDLER) {
    }
}

void RequestTimeStep(float deltaDegrees) {
    void* pEnv = GetEnvironmentInstance();
    if (!pEnv) return;

    float base = Config::IsSmoothTransitioning.load()
        ? Config::TargetAngle.load()
        : *(float*)((uintptr_t)pEnv + Config::OffsetTimeAngle);

    float newTarget = base + deltaDegrees;
    while (newTarget < 0.0f) newTarget += 360.0f;
    while (newTarget >= 360.0f) newTarget -= 360.0f;

    Config::TargetAngle.store(newTarget);
    Config::IsSmoothTransitioning.store(true);

    int hours = 0, minutes = 0;
    FormatTimeDisplay(newTarget, hours, minutes);
    std::cout << "time: step changed -> " << (deltaDegrees >= 0 ? "+" : "") << std::fixed << std::setprecision(1) << deltaDegrees
              << " deg (" << std::setfill('0') << std::setw(2) << hours << ":"
              << std::setfill('0') << std::setw(2) << minutes << ")\n";
}

void RequestTimeHour(int newHour) {
    float targetAngle = static_cast<float>(newHour * 15.0f);
    while (targetAngle < 0.0f) targetAngle += 360.0f;
    while (targetAngle >= 360.0f) targetAngle -= 360.0f;

    Config::TargetAngle.store(targetAngle);
    Config::IsSmoothTransitioning.store(true);

    std::cout << "ui: Time of Day changed -> " << newHour << ":00\n";
}

static void DecodeInternalOffsets(uintptr_t funcAddr) {
    if (!funcAddr) return;

    __try {
        const uint8_t* p = reinterpret_cast<const uint8_t*>(funcAddr);
        bool foundMode = false, foundAngle = false, foundAxis = false, foundQuat = false;

        for (size_t i = 0; i < 240; ++i) {
            // C7 81 [disp32] 02 00 00 00 -> mov dword ptr [rcx+disp32], 2
            if (!foundMode && p[i] == 0xC7 && p[i + 1] == 0x81 &&
                *reinterpret_cast<const uint32_t*>(p + i + 6) == 2) {
                Config::OffsetTimeMode = *reinterpret_cast<const uint32_t*>(p + i + 2);
                foundMode = true;
            }

            // F3 0F 58 ?? [disp32] -> addss xmmX, dword ptr [rcx+disp32]
            if (!foundAngle && p[i] == 0xF3 && p[i + 1] == 0x0F && p[i + 2] == 0x58) {
                uint8_t modrm = p[i + 3];
                if ((modrm & 0xC0) == 0x80) {
                    Config::OffsetTimeAngle = *reinterpret_cast<const uint32_t*>(p + i + 4);
                    foundAngle = true;
                }
            }

            // 0F 10 ?? [disp32] -> movups xmmX, [rbx+disp32]
            if (!foundAxis && i > 30 && p[i] == 0x0F && p[i + 1] == 0x10) {
                uint8_t modrm = p[i + 2];
                if ((modrm & 0xC0) == 0x80) {
                    Config::OffsetPlanetAxis = *reinterpret_cast<const uint32_t*>(p + i + 3);
                    foundAxis = true;
                }
            }

            // 0F 11 ?? [disp32] -> movups [rbx+disp32], xmm0
            if (!foundQuat && i > 100 && p[i] == 0x0F && p[i + 1] == 0x11) {
                uint8_t modrm = p[i + 2];
                if ((modrm & 0xC0) == 0x80) {
                    Config::OffsetSunQuat = *reinterpret_cast<const uint32_t*>(p + i + 3);
                    foundQuat = true;
                }
            }
        }
    }
    __except (EXCEPTION_EXECUTE_HANDLER) {
    }
}

static bool ApplyHooks() {
    std::cout << "time: scanning for global app wrapper signature\n";

    // sub_7FF6A41D4520: mov rcx, [rip+disp32]; movss xmm1, [rip+disp32]; add rcx, disp32; jmp rel32
    uintptr_t sigWrapper = FindPattern("NMS.exe", Config::SigGlobalAppWrapper);
    if (sigWrapper) {
        std::cout << "time: signature match @ 0x" << std::hex << sigWrapper << std::dec << "\n";

        int32_t relApp = *reinterpret_cast<int32_t*>(sigWrapper + 3);
        Config::pGlobalAppPtr = reinterpret_cast<uintptr_t*>(sigWrapper + 7 + relApp);
        Config::OffsetEnvFromApp = *reinterpret_cast<uint32_t*>(sigWrapper + 18);

        int32_t relJmp = *reinterpret_cast<int32_t*>(sigWrapper + 23);
        uintptr_t targetSetTime = sigWrapper + 27 + relJmp;
        fnSetTimeOfDay = reinterpret_cast<tSetTimeOfDayDelta>(targetSetTime);
        Config::AddrSetTimeOfDay = targetSetTime;

        std::cout << "time: global app pointer @ 0x" << std::hex << (uintptr_t)Config::pGlobalAppPtr
                  << " (environment offset 0x" << Config::OffsetEnvFromApp << ")" << std::dec << "\n";
        std::cout << "time: set time of day @ 0x" << std::hex << targetSetTime << std::dec << "\n";

        DWORD64 imageBase = 0;
        PRUNTIME_FUNCTION pFunc = RtlLookupFunctionEntry(static_cast<DWORD64>(targetSetTime), &imageBase, nullptr);
        if (pFunc) {
            uintptr_t funcStart = static_cast<uintptr_t>(imageBase + pFunc->BeginAddress);
            uintptr_t funcEnd = static_cast<uintptr_t>(imageBase + pFunc->EndAddress);
            size_t funcSize = funcEnd - funcStart;
            std::cout << "time: function bounds 0x" << std::hex << funcStart
                      << "-0x" << funcEnd << " (" << std::dec << funcSize << " bytes)\n";
        }
    }

    if (!fnSetTimeOfDay) {
        std::cout << "time: scanning for flexible set time signature\n";
        Config::AddrSetTimeOfDay = FindPattern("NMS.exe", Config::SigSetTimeOfDayFlexible);
        if (Config::AddrSetTimeOfDay) {
            fnSetTimeOfDay = reinterpret_cast<tSetTimeOfDayDelta>(Config::AddrSetTimeOfDay);
            std::cout << "time: signature match @ 0x" << std::hex << Config::AddrSetTimeOfDay << std::dec << "\n";
        }
    }

    if (!fnSetTimeOfDay || !Config::pGlobalAppPtr) {
        std::cout << "time: signature not found\n";
        return false;
    }

    DecodeInternalOffsets(Config::AddrSetTimeOfDay);
    std::cout << "time: offsets -> mode 0x" << std::hex << Config::OffsetTimeMode
              << ", angle 0x" << Config::OffsetTimeAngle
              << ", sun quat 0x" << Config::OffsetSunQuat
              << ", planet axis 0x" << Config::OffsetPlanetAxis << std::dec << "\n";

    std::cout << "time: hook live @ 0x" << std::hex << Config::AddrSetTimeOfDay << std::dec << "\n";

    return true;
}

static void KeyPollLoop() {
    LARGE_INTEGER qpcFreq;
    QueryPerformanceFrequency(&qpcFreq);

    LARGE_INTEGER lastQPC;
    QueryPerformanceCounter(&lastQPC);

    float hotkeyCooldown = 0.0f;
    double deltaAccumulator = 0.0;
    uint64_t lastConfigCheckTime = 0;

    while (Config::bInitialized.load()) {
        LARGE_INTEGER nowQPC;
        QueryPerformanceCounter(&nowQPC);

        double dt = static_cast<double>(nowQPC.QuadPart - lastQPC.QuadPart) / static_cast<double>(qpcFreq.QuadPart);
        lastQPC = nowQPC;

        if (dt < 0.0) dt = 0.0;
        if (dt > 0.05) dt = 0.05;

        uint64_t nowMs = GetTickCount64();
        if (nowMs - lastConfigCheckTime >= 1000) {
            lastConfigCheckTime = nowMs;
            if (HasConfigChanged()) {
                LoadConfig();
                std::cout << "init: configuration reloaded from disk\n";
            }
        }

        if (Config::EnableHotkeys.load()) {
            if (hotkeyCooldown > 0.0f) {
                hotkeyCooldown -= static_cast<float>(dt);
            } else {
                bool isPlusPressed = (GetAsyncKeyState(Config::AdvanceTimeKey) & 0x8000) != 0;
                bool isMinusPressed = (GetAsyncKeyState(Config::RewindTimeKey) & 0x8000) != 0;

                float stepMins = static_cast<float>(Config::StepMinutes.load());
                float stepDegrees = (stepMins / 60.0f) * 15.0f;

                if (isPlusPressed) {
                    RequestTimeStep(stepDegrees);
                    hotkeyCooldown = 0.18f;
                } else if (isMinusPressed) {
                    RequestTimeStep(-stepDegrees);
                    hotkeyCooldown = 0.18f;
                }
            }
        }

        void* pEnv = GetEnvironmentInstance();
        if (pEnv) {
            float curAngle = *(float*)((uintptr_t)pEnv + Config::OffsetTimeAngle);
            while (curAngle < 0.0f) curAngle += 360.0f;
            while (curAngle >= 360.0f) curAngle -= 360.0f;

            if (Config::IsSmoothTransitioning.load()) {
                float target = Config::TargetAngle.load();

                float diff = target - curAngle;
                while (diff > 180.0f) diff -= 360.0f;
                while (diff < -180.0f) diff += 360.0f;

                if (std::abs(diff) <= 0.05f) {
                    __try {
                        if (fnSetTimeOfDay) {
                            fnSetTimeOfDay(pEnv, diff);
                        }
                        Config::FrozenAngle.store(target);
                    }
                    __except (EXCEPTION_EXECUTE_HANDLER) {
                    }
                    Config::IsSmoothTransitioning.store(false);
                    deltaAccumulator = 0.0;
                } else {
                    float speed = std::abs(diff) * 14.0f;
                    if (speed < 20.0f) speed = 20.0f;
                    if (speed > 160.0f) speed = 160.0f;

                    float step = speed * static_cast<float>(dt);
                    if (step > std::abs(diff)) step = std::abs(diff);

                    float applyDelta = (diff > 0.0f) ? step : -step;
                    __try {
                        if (fnSetTimeOfDay) {
                            fnSetTimeOfDay(pEnv, applyDelta);
                        }
                        Config::FrozenAngle.store(*(float*)((uintptr_t)pEnv + Config::OffsetTimeAngle));
                    }
                    __except (EXCEPTION_EXECUTE_HANDLER) {
                    }
                }
            }
            else {
                bool isFrozen = Config::FreezeTime.load();
                float speedMul = Config::TimeSpeedMultiplier.load();

                if (isFrozen || speedMul <= 0.0001f) {
                    __try {
                        *(float*)((uintptr_t)pEnv + Config::OffsetTimeAngle) = Config::FrozenAngle.load();
                        *(int*)((uintptr_t)pEnv + Config::OffsetTimeMode) = 2;
                    }
                    __except (EXCEPTION_EXECUTE_HANDLER) {
                    }
                    deltaAccumulator = 0.0;
                }
                else {
                    constexpr double kVanillaDegPerSec = 0.20;
                    double currentRate = speedMul * kVanillaDegPerSec;

                    deltaAccumulator += currentRate * dt;

                    if (deltaAccumulator >= 0.00025) {
                        float applyDelta = static_cast<float>(deltaAccumulator);
                        deltaAccumulator = 0.0;

                        __try {
                            if (fnSetTimeOfDay) {
                                fnSetTimeOfDay(pEnv, applyDelta);
                            }
                            Config::FrozenAngle.store(*(float*)((uintptr_t)pEnv + Config::OffsetTimeAngle));
                        }
                        __except (EXCEPTION_EXECUTE_HANDLER) {
                        }
                    }
                }
            }
        }

        Sleep(1);
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

    std::cout << "hooks: patching timeofday bytecode\n";
    if (!ApplyHooks()) {
        std::cout << "time: hooks failed to apply\n";
        return 1;
    }

    std::cout << "ui: hooking options menu\n";
    if (!TimeOfDayUI::Initialize()) {
        std::cout << "ui: failed to hook options menu\n";
    }

    Config::bInitialized.store(true);
    std::cout << "main: entering key poll loop\n\n";
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
        Logger::Shutdown();
    }
    return TRUE;
}
