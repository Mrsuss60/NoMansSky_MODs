#pragma once
#include <windows.h>
#include <cstdint>
#include <atomic>

namespace Config {
    inline std::atomic<bool> bInitialized{ false };
    inline bool EnableConsole = 0;

    inline uintptr_t AddrSetTimeOfDay = 0;
    inline uintptr_t* pGlobalAppPtr = nullptr;
    inline void* pActiveEnvironment = nullptr;

    inline std::atomic<bool> FreezeTime{ false };
    inline std::atomic<float> FrozenAngle{ 180.0f };
    inline std::atomic<float> TimeSpeedMultiplier{ 1.0f };
    inline std::atomic<int> StepMinutes{ 15 };
    inline std::atomic<bool> EnableHotkeys{ true };

    inline int AdvanceTimeKey = VK_ADD;
    inline int RewindTimeKey  = VK_SUBTRACT;

    inline std::atomic<float> TargetAngle{ 180.0f };
    inline std::atomic<bool>  IsSmoothTransitioning{ false };

    // sub_7FF6A41D4520 (PhotoModeControl_TimeOfDay vftable[3])
    inline const char* SigGlobalAppWrapper = "48 8B 0D ?? ?? ?? ?? F3 0F 10 0D ?? ?? ?? ?? 48 81 C1 ?? ?? ?? ?? E9";

    // sub_7FF6A41DF180 (solar angle wrap logic)
    inline const char* SigSetTimeOfDayFlexible = "C7 81 ?? ?? 00 00 02 00 00 00 F3 0F 58 ?? ?? ?? 00 00";

    // sub_7FF6A3AA9480 (GcOptionsPage::EndOptionsPage)
    inline const char* SigEndOptionsPage = "48 89 5C 24 10 48 89 6C 24 18 56 57 41 55 41 56 41 57 48 83 EC 40 48 8B 81";

    inline uint32_t OffsetTimeAngle  = 0x4F0;
    inline uint32_t OffsetSunQuat    = 0x500;
    inline uint32_t OffsetPlanetAxis = 0x520;
    inline uint32_t OffsetTimeMode   = 0x600;
    inline uint32_t OffsetEnvFromApp = 0x55F1F0;
}

void* GetEnvironmentInstance();
void AdjustTimeOfDay(float deltaDegrees);
void RequestTimeStep(float deltaDegrees);
void RequestTimeHour(int newHour);

