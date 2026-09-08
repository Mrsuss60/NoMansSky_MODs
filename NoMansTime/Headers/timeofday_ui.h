#pragma once
#include <windows.h>
#include <cstdint>

namespace TimeOfDayUI {
    struct SliderExtraParams {
        int32_t stepConfig;
        int32_t flags;
        const char* suffix;
        int64_t reserved;
    };

    typedef void(__fastcall* tAddSectionHeader)(void* pUIContext, const char* labelKey, __int64 iconId);
    typedef char(__fastcall* tAddToggleOption)(
        void* pUIContext,
        const char* labelKey,
        const char* descKey,
        char currentValue,
        char defaultValue,
        const char** pLabelsArray
    );
    typedef int(__fastcall* tAddIntSlider)(
        void* pUIContext,
        const char* labelKey,
        const char* descKey,
        int currentValue,
        int defaultValue,
        int minValue,
        int maxValue,
        SliderExtraParams* pExtraParams
    );
    typedef void(__fastcall* tStartCyclicOption)(
        void* pUIContext,
        const char* labelKey,
        const char* descKey,
        int currentValue,
        int defaultValue,
        int* pFlags
    );
    typedef void(__fastcall* tAddCyclicOptionEntry)(
        void* pUIContext,
        const char* entryLabel,
        __int64 entryIndex
    );
    typedef int(__fastcall* tEndCyclicOption)(
        void* pUIContext
    );
    typedef int(__fastcall* tEndOptionsPage)(void* pUIContext, unsigned int hasChanges, __int64 reserved);

    bool Initialize();
    void RenderCustomTimeOfDayOptions(void* pUIContext);
}
