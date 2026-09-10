#include "timeofday_ui.h"
#include "globals.h"
#include "g_memory.h"
#include "config_file.h"
#include "logger.h"
#include <iostream>
#include <iomanip>
#include <cmath>
#include <algorithm>
#include <cstdarg>

namespace TimeOfDayUI {

    static uintptr_t s_CallSiteEndOptions = 0;
    static void* s_pRelayTrampoline = nullptr;
    static uintptr_t s_CallSiteSprintf = 0;
    static void* s_pSprintfTrampoline = nullptr;

    static tAddSectionHeader fnAddSectionHeader = nullptr;
    static tAddToggleOption  fnAddToggleOption = nullptr;
    static tAddIntSlider     fnAddIntSlider = nullptr;
    static tEndOptionsPage   fnEndOptionsPage = nullptr;

    static bool s_FormattingSpeedSlider = false;
    static char s_CustomSpeedText[32] = "";

    static const float kSpeedPresets[] = {
        0.00f, 0.25f, 0.50f, 0.75f, 1.00f,
        2.00f, 5.00f, 10.00f, 20.00f, 30.00f, 40.00f, 50.00f
    };
    constexpr int kSpeedPresetCount = sizeof(kSpeedPresets) / sizeof(kSpeedPresets[0]);

    static int __cdecl Detour_SliderSprintf(char* buffer, const char* format, ...) {
        if (s_FormattingSpeedSlider && s_CustomSpeedText[0] != '\0') {
            strcpy_s(buffer, 32, s_CustomSpeedText);
            return static_cast<int>(strlen(buffer));
        }
        va_list args;
        va_start(args, format);
        int res = vsnprintf_s(buffer, 32, _TRUNCATE, format, args);
        va_end(args);
        return res;
    }

    void RenderCustomTimeOfDayOptions(void* pUIContext) {
        if (!pUIContext) return;

        static bool s_LoggedRender = false;
        if (!s_LoggedRender) {
            LOG << "ui: options page rendered\n";
            s_LoggedRender = true;
        }

        __try {
            fnAddSectionHeader(pUIContext, "NO MAN'S TIME", 2);

            const char* toggleLabels[2] = { "UI_ENABLED", "UI_DISABLED" };

            char curFreeze = Config::FreezeTime.load() ? 1 : 0;
            char newFreeze = fnAddToggleOption(
                pUIContext,
                "FREEZE DAY/NIGHT CYCLE",
                "Freeze or unfreeze day/night cycle",
                curFreeze,
                0,
                toggleLabels
            );

            if (newFreeze != curFreeze) {
                bool bFreeze = (newFreeze != 0);
                Config::FreezeTime.store(bFreeze);
                if (bFreeze) {
                    Config::TimeSpeedMultiplier.store(0.0f);
                    void* pEnv = GetEnvironmentInstance();
                    if (pEnv) {
                        float curAngle = *(float*)((uintptr_t)pEnv + Config::OffsetTimeAngle);
                        Config::FrozenAngle.store(curAngle);
                    }
                } else {
                    if (Config::TimeSpeedMultiplier.load() <= 0.0001f) {
                        Config::TimeSpeedMultiplier.store(1.0f);
                    }
                }
                SaveConfig();
            }

            void* pEnv = GetEnvironmentInstance();
            int curHour = 12;
            if (pEnv) {
                float angle = Config::IsSmoothTransitioning.load()
                    ? Config::TargetAngle.load()
                    : *(float*)((uintptr_t)pEnv + Config::OffsetTimeAngle);
                while (angle < 0.0f) angle += 360.0f;
                while (angle >= 360.0f) angle -= 360.0f;
                curHour = static_cast<int>(std::floor(angle / 15.0f));
                if (curHour > 24) curHour = 24;
            }

            SliderExtraParams hourParams{};
            hourParams.stepConfig = 1;
            hourParams.flags = 1;
            hourParams.suffix = ":00";
            hourParams.reserved = 0;

            int newHour = fnAddIntSlider(
                pUIContext,
                "TIME OF DAY (HOUR)",
                "Change the time of day",
                curHour,
                12,
                0,
                24,
                &hourParams
            );

            if (newHour != curHour && pEnv) {
                RequestTimeHour(newHour);
            }

            float curMult = Config::TimeSpeedMultiplier.load();
            int curIndex = 4;
            if (Config::FreezeTime.load() || curMult <= 0.001f) {
                curIndex = 0;
            } else {
                float bestDiff = 999.0f;
                int bestIdx = 4;
                for (int i = 0; i < kSpeedPresetCount; ++i) {
                    float diff = std::abs(kSpeedPresets[i] - curMult);
                    if (diff < bestDiff) {
                        bestDiff = diff;
                        bestIdx = i;
                    }
                }
                curIndex = bestIdx;
            }

            if (curIndex == 0) {
                strcpy_s(s_CustomSpeedText, "0.0x (frozen)");
            } else if (curIndex == 1) {
                strcpy_s(s_CustomSpeedText, "0.25x");
            } else if (curIndex == 2) {
                strcpy_s(s_CustomSpeedText, "0.50x");
            } else if (curIndex == 3) {
                strcpy_s(s_CustomSpeedText, "0.75x");
            } else if (curIndex == 4) {
                strcpy_s(s_CustomSpeedText, "1.0x (default)");
            } else {
                sprintf_s(s_CustomSpeedText, "%.1fx", kSpeedPresets[curIndex]);
            }

            SliderExtraParams speedParams{};
            speedParams.stepConfig = 1;
            speedParams.flags = 1;
            speedParams.suffix = "";
            speedParams.reserved = 0;

            s_FormattingSpeedSlider = true;
            int newIndex = fnAddIntSlider(
                pUIContext,
                "DAY/NIGHT CYCLE SPEED",
                "Speed multiplier for the day/night progression (0.0x to 50.0x)",
                curIndex,
                4,
                0,
                kSpeedPresetCount - 1,
                &speedParams
            );
            s_FormattingSpeedSlider = false;

            if (newIndex != curIndex) {
                float newMul = kSpeedPresets[newIndex];
                Config::TimeSpeedMultiplier.store(newMul);

                if (newIndex == 0) {
                    Config::FreezeTime.store(true);
                    if (pEnv) {
                        float curAngle = *(float*)((uintptr_t)pEnv + Config::OffsetTimeAngle);
                        Config::FrozenAngle.store(curAngle);
                    }
                } else {
                    Config::FreezeTime.store(false);
                }
                SaveConfig();
            }

            SliderExtraParams stepParams{};
            stepParams.stepConfig = 1;
            stepParams.flags = 1;
            stepParams.suffix = "m";
            stepParams.reserved = 0;

            int curStep = Config::StepMinutes.load();
            int newStep = fnAddIntSlider(
                pUIContext,
                "STEP SIZE (MINUTES)",
                "Minutes changed per hotkey press",
                curStep,
                15,
                1,
                60,
                &stepParams
            );

            if (newStep != curStep) {
                Config::StepMinutes.store(newStep);
                SaveConfig();
            }
        }
        __except (EXCEPTION_EXECUTE_HANDLER) {
            LOG << "ui: exception caught during options render\n";
        }
    }

    static int __fastcall Detour_EndGeneralOptionsCallSite(void* pUIContext, unsigned int hasChanges, __int64 reserved) {
        RenderCustomTimeOfDayOptions(pUIContext);
        if (fnEndOptionsPage) {
            return fnEndOptionsPage(pUIContext, hasChanges, reserved);
        }
        return 0;
    }

    bool Initialize() {
        const char* modName = "NMS.exe";

        LOG << "ui: resolving dynamic string cross-references\n";

        uintptr_t strMisc = FindString(modName, "UI_OPTIONS_MISC");
        if (!strMisc) {
            LOG << "ui: failed to locate UI_OPTIONS_MISC\n";
            return false;
        }

        uintptr_t xrefMisc = FindRipRef(modName, strMisc);
        if (!xrefMisc) {
            LOG << "ui: failed to locate xref for UI_OPTIONS_MISC\n";
            return false;
        }

        DWORD64 imageBase = 0;
        PRUNTIME_FUNCTION pFunc = RtlLookupFunctionEntry(static_cast<DWORD64>(xrefMisc), &imageBase, nullptr);
        if (!pFunc) {
            LOG << "ui: RtlLookupFunctionEntry failed for options function\n";
            return false;
        }

        uintptr_t funcStart = static_cast<uintptr_t>(imageBase + pFunc->BeginAddress);
        uintptr_t funcEnd = static_cast<uintptr_t>(imageBase + pFunc->EndAddress);
        size_t funcSize = static_cast<size_t>(funcEnd - funcStart);
        LOG << "ui: options function bounds 0x" << std::hex << funcStart << "-0x" << funcEnd << " (" << std::dec << funcSize << " bytes)\n";

        uintptr_t callHeader = FindNthCallForward(xrefMisc, 0x100, 1);
        if (!callHeader || callHeader >= funcEnd) return false;
        fnAddSectionHeader = reinterpret_cast<tAddSectionHeader>(ResolveCallTarget(callHeader));
        LOG << "ui: AddSectionHeader @ 0x" << std::hex << (uintptr_t)fnAddSectionHeader << std::dec << "\n";

        uintptr_t strHeadBob = FindString(modName, "UI_HEAD_BOB");
        if (strHeadBob) {
            uintptr_t xrefHeadBob = FindRipRef(modName, strHeadBob);
            if (xrefHeadBob) {
                uintptr_t callToggle = FindNthCallForward(xrefHeadBob, 0x100, 1);
                if (callToggle) {
                    fnAddToggleOption = reinterpret_cast<tAddToggleOption>(ResolveCallTarget(callToggle));
                    LOG << "ui: AddToggleOption @ 0x" << std::hex << (uintptr_t)fnAddToggleOption << std::dec << "\n";
                }
            }
        }

        uintptr_t strShake = FindString(modName, "UI_OPTIONS_CAMERA_SHAKE_L");
        if (strShake) {
            uintptr_t xrefShake = FindRipRef(modName, strShake);
            if (xrefShake) {
                uintptr_t callSlider = FindNthCallForward(xrefShake, 0x100, 1);
                if (callSlider) {
                    fnAddIntSlider = reinterpret_cast<tAddIntSlider>(ResolveCallTarget(callSlider));
                    LOG << "ui: AddIntSlider @ 0x" << std::hex << (uintptr_t)fnAddIntSlider << std::dec << "\n";
                }
            }
        }

        if (fnAddIntSlider) {
            const uint8_t* pCode = reinterpret_cast<const uint8_t*>(fnAddIntSlider);
            for (size_t i = 0; i < 0x250; ++i) {
                if (pCode[i] == 0x48 && pCode[i + 1] == 0x8D && pCode[i + 2] == 0x15) {
                    int32_t disp = *reinterpret_cast<const int32_t*>(&pCode[i + 3]);
                    const char* strTarget = reinterpret_cast<const char*>(&pCode[i + 7] + disp);
                    bool isFormatString = false;
                    __try {
                        if (strncmp(strTarget, "%d%s", 4) == 0) {
                            isFormatString = true;
                        }
                    }
                    __except (EXCEPTION_EXECUTE_HANDLER) {
                        isFormatString = false;
                    }

                    if (isFormatString) {
                        for (size_t j = i + 7; j < i + 40; ++j) {
                            if (pCode[j] == 0xE8) {
                                s_CallSiteSprintf = reinterpret_cast<uintptr_t>(&pCode[j]);
                                break;
                            }
                        }
                        if (s_CallSiteSprintf) break;
                    }
                }
            }

            if (s_CallSiteSprintf) {
                s_pSprintfTrampoline = AllocateNearAddress(s_CallSiteSprintf, 0x40);
                if (s_pSprintfTrampoline) {
                    int64_t dist = reinterpret_cast<uintptr_t>(s_pSprintfTrampoline) - (s_CallSiteSprintf + 5);
                    if (dist >= INT32_MIN && dist <= INT32_MAX) {
                        uint8_t stub[14] = { 0xFF, 0x25, 0x00, 0x00, 0x00, 0x00 };
                        uintptr_t detour = reinterpret_cast<uintptr_t>(&Detour_SliderSprintf);
                        memcpy(&stub[6], &detour, sizeof(uintptr_t));

                        DWORD oldProtect;
                        VirtualProtect(s_pSprintfTrampoline, sizeof(stub), PAGE_READWRITE, &oldProtect);
                        memcpy(s_pSprintfTrampoline, stub, sizeof(stub));
                        VirtualProtect(s_pSprintfTrampoline, sizeof(stub), PAGE_EXECUTE_READ, &oldProtect);
                        FlushInstructionCache(GetCurrentProcess(), s_pSprintfTrampoline, sizeof(stub));

                        VirtualProtect(reinterpret_cast<void*>(s_CallSiteSprintf), 5, PAGE_EXECUTE_READWRITE, &oldProtect);
                        *reinterpret_cast<int32_t*>(s_CallSiteSprintf + 1) = static_cast<int32_t>(dist);
                        VirtualProtect(reinterpret_cast<void*>(s_CallSiteSprintf), 5, oldProtect, &oldProtect);
                        FlushInstructionCache(GetCurrentProcess(), reinterpret_cast<void*>(s_CallSiteSprintf), 5);

                        LOG << "ui: slider format hook live @ 0x" << std::hex << s_CallSiteSprintf
                            << " -> trampoline 0x" << (uintptr_t)s_pSprintfTrampoline << std::dec << "\n";
                    }
                }
            }
        }

        uintptr_t strCredits = FindString(modName, "OPTION_CREDITS");
        uintptr_t xrefCredits = strCredits ? FindRipRefInRange(strCredits, funcStart, funcEnd) : 0;

        // sub_7FF6A3AA9480 (GcOptionsPage::EndOptionsPage)
        uintptr_t addrEndPage = FindPattern(modName, Config::SigEndOptionsPage);
        if (addrEndPage) {
            fnEndOptionsPage = reinterpret_cast<tEndOptionsPage>(addrEndPage);
            for (uintptr_t curr = funcStart; curr + 5 <= funcEnd; ++curr) {
                if (*reinterpret_cast<uint8_t*>(curr) == 0xE8) {
                    uintptr_t target = ResolveCallTarget(curr);
                    if (target == reinterpret_cast<uintptr_t>(fnEndOptionsPage)) {
                        s_CallSiteEndOptions = curr;
                        break;
                    }
                }
            }
        }

        // mov edx, 1; mov rcx, rbx; call ...; sub eax, esi
        if (!s_CallSiteEndOptions && xrefCredits) {
            for (uintptr_t curr = xrefCredits; curr + 12 <= funcEnd; ++curr) {
                if (*reinterpret_cast<uint8_t*>(curr) == 0xBA &&
                    *reinterpret_cast<uint32_t*>(curr + 1) == 1 &&
                    *reinterpret_cast<uint8_t*>(curr + 5) == 0x48 &&
                    *reinterpret_cast<uint8_t*>(curr + 6) == 0x8B &&
                    *reinterpret_cast<uint8_t*>(curr + 7) == 0xCB &&
                    *reinterpret_cast<uint8_t*>(curr + 8) == 0xE8) {
                    s_CallSiteEndOptions = curr + 8;
                    if (!fnEndOptionsPage) {
                        fnEndOptionsPage = reinterpret_cast<tEndOptionsPage>(ResolveCallTarget(s_CallSiteEndOptions));
                    }
                    break;
                }
            }
        }

        if (!s_CallSiteEndOptions && xrefCredits) {
            s_CallSiteEndOptions = FindNthCallForward(xrefCredits, 0x100, 4);
            if (s_CallSiteEndOptions && !fnEndOptionsPage) {
                fnEndOptionsPage = reinterpret_cast<tEndOptionsPage>(ResolveCallTarget(s_CallSiteEndOptions));
            }
        }

        if (!s_CallSiteEndOptions || s_CallSiteEndOptions >= funcEnd || *reinterpret_cast<uint8_t*>(s_CallSiteEndOptions) != 0xE8) {
            LOG << "ui: failed to locate EndOptionsPage call site\n";
            return false;
        }

        if (!fnAddSectionHeader || !fnAddToggleOption || !fnAddIntSlider || !fnEndOptionsPage) {
            LOG << "ui: UI function pointers failed to resolve\n";
            return false;
        }

        s_pRelayTrampoline = AllocateNearAddress(s_CallSiteEndOptions, 0x40);
        if (!s_pRelayTrampoline) return false;

        int64_t fullDistance = reinterpret_cast<uintptr_t>(s_pRelayTrampoline) - (s_CallSiteEndOptions + 5);
        if (fullDistance < INT32_MIN || fullDistance > INT32_MAX) return false;

        uint8_t relayStub[14] = { 0xFF, 0x25, 0x00, 0x00, 0x00, 0x00 };
        uintptr_t detourAddr = reinterpret_cast<uintptr_t>(&Detour_EndGeneralOptionsCallSite);
        memcpy(&relayStub[6], &detourAddr, sizeof(uintptr_t));

        DWORD oldProtect;
        VirtualProtect(s_pRelayTrampoline, sizeof(relayStub), PAGE_READWRITE, &oldProtect);
        memcpy(s_pRelayTrampoline, relayStub, sizeof(relayStub));
        VirtualProtect(s_pRelayTrampoline, sizeof(relayStub), PAGE_EXECUTE_READ, &oldProtect);
        FlushInstructionCache(GetCurrentProcess(), s_pRelayTrampoline, sizeof(relayStub));

        VirtualProtect(reinterpret_cast<void*>(s_CallSiteEndOptions), 5, PAGE_EXECUTE_READWRITE, &oldProtect);
        *reinterpret_cast<int32_t*>(s_CallSiteEndOptions + 1) = static_cast<int32_t>(fullDistance);
        VirtualProtect(reinterpret_cast<void*>(s_CallSiteEndOptions), 5, oldProtect, &oldProtect);
        FlushInstructionCache(GetCurrentProcess(), reinterpret_cast<void*>(s_CallSiteEndOptions), 5);

        LOG << "ui: options hook live @ 0x" << std::hex << s_CallSiteEndOptions
            << " -> relay 0x" << (uintptr_t)s_pRelayTrampoline
            << " -> fnEndOptionsPage @ 0x" << (uintptr_t)fnEndOptionsPage << std::dec << "\n";

        return true;
    }
}
