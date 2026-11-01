#pragma once
#include <windows.h>
#include <atomic>
#include <cstdint>
#include <cstring>

namespace Config {
    // Spacewalking, underwater combat, and derelict freighter templates don't start with "CHAR"
    inline bool IsOnFootTag(const char* tag) {
        if (std::strncmp(tag, "CHAR", 4) == 0) return true;
        if (std::strcmp(tag, "SPACECOMBAT") == 0) return true;
        if (std::strcmp(tag, "WETCOMBAT") == 0) return true;
        if (std::strcmp(tag, "FREIGHTCOMBAT") == 0) return true;
        if (std::strcmp(tag, "ABANDFREIGHT") == 0) return true;
        return false;
    }

    inline float BlendSmooth(float start, float target, float t) {
        t = t < 0.0f ? 0.0f : (t > 1.0f ? 1.0f : t);
        float eased = t * t * (3.0f - 2.0f * t);
        return start + (target - start) * eased;
    }

    inline std::atomic<float> CustomX = 22.54f;
    inline std::atomic<float> CustomHeight = -0.67f;
    inline std::atomic<float> CustomDist = 3.02f;
    inline float CorvetteInteriorDistance = 3.40f;
    inline float CorvetteInteriorDistanceTolerance = 0.02f;
    inline float CorvetteSafeCustomDistance = 3.41f;
    inline float Step = 0.18f;
    inline std::atomic<bool> EnableCameraSmoothing = false;
    inline std::atomic<bool> EnableHotkeys = true;
    inline bool EnableConsole = 0;

    inline int ToggleKey = VK_F3;
    inline int IncDistKey = VK_F4;
    inline int DecDistKey = VK_F5;
    inline int IncWidthKey = VK_F7;
    inline int DecWidthKey = VK_F6;
    inline int IncHeightKey = VK_F9;
    inline int DecHeightKey = VK_F8;

    inline uint32_t StructStride = 0x110;
    inline int CopiesXYZ = 2;
    inline int CopiesSmoothing = 2;
    inline int CopiesCollision = 2;

    inline uint32_t OffsetDist = 0x508;
    inline uint32_t OffsetX = 0x564;
    inline uint32_t OffsetHeight = 0x568;
    inline uint32_t OffsetCameraSmoothing = 0x278;
    inline uint32_t OffsetSprintCameraS = 0x504;
    inline uint32_t OffsetCollision = 0x510;

    inline const char* SigCamera = "F3 0F 11 8B 08 05 00 00 F3 0F 10 ?? ?? ??";
    inline const char* SigCameraShort = "F3 0F 11 8B 08 05 00 00";
    inline const char* SigCameraLegacy = "F3 0F 11 8B 98 04 00 00";
    inline const char* SigCollisionRead1 = "F3 0F 59 83 10 05 00 00";
    inline const char* SigCollisionRead2 = "F3 0F 59 93 10 05 00 00";
    inline const char* SigCameraSmoothing2 = "F3 0F 11 9E 78 02 00 00";

    inline uint32_t OffsetCharacterRigTag = 0x4E8;
    inline int CorvetteExitTransitionSteps = 35;

    inline std::atomic<bool> EnableCorvetteInteriorGuard = true;

    inline std::atomic<bool> bModActive = true;
    inline std::atomic<bool> bInitialized = false;
    inline uintptr_t GlobalCameraPtr = 0;
    inline uintptr_t AddrCameraHook = 0;
    inline uintptr_t AddrCodecave = 0;
    inline uintptr_t AddrCollision1 = 0;
    inline uintptr_t AddrCollision2 = 0;
    inline uintptr_t AddrCameraSmoothing2 = 0;

    inline unsigned char OrigCamera[8] = { 0xF3, 0x0F, 0x11, 0x8B, 0x08, 0x05, 0x00, 0x00 };
    inline unsigned char OrigCollision1[8] = { 0xF3, 0x0F, 0x59, 0x83, 0x10, 0x05, 0x00, 0x00 };
    inline unsigned char OrigCollision2[8] = { 0xF3, 0x0F, 0x59, 0x93, 0x10, 0x05, 0x00, 0x00 };

    inline unsigned char PatchXorpsXmm0[8] = { 0x0F, 0x57, 0xC0, 0x90, 0x90, 0x90, 0x90, 0x90 };
    inline unsigned char PatchXorpsXmm2[8] = { 0x0F, 0x57, 0xD2, 0x90, 0x90, 0x90, 0x90, 0x90 };
    inline unsigned char PatchCameraJmp[8] = { 0xE9, 0x00, 0x00, 0x00, 0x00, 0x90, 0x90, 0x90 };

    inline unsigned char OrigCameraSmoothing2[8] = { 0xF3, 0x0F, 0x11, 0x9E, 0x78, 0x02, 0x00, 0x00 };
    inline unsigned char PatchNop8[8] = { 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90 };
}