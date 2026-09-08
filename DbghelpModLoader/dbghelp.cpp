#include "pch.h"
#include <process.h>
#include <string>
#include <vector>

extern "C" void* g_pRealFuncs[257] = { nullptr };

static const char* const g_FuncNames[242] = {
    "DbgHelpCreateUserDump",
    "DbgHelpCreateUserDumpW",
    "EnumDirTree",
    "EnumDirTreeW",
    "EnumerateLoadedModules",
    "EnumerateLoadedModules64",
    "EnumerateLoadedModulesEx",
    "EnumerateLoadedModulesExW",
    "EnumerateLoadedModulesW64",
    "ExtensionApiVersion",
    "FindDebugInfoFile",
    "FindDebugInfoFileEx",
    "FindDebugInfoFileExW",
    "FindExecutableImage",
    "FindExecutableImageEx",
    "FindExecutableImageExW",
    "FindFileInPath",
    "FindFileInSearchPath",
    "GetSymLoadError",
    "GetTimestampForLoadedLibrary",
    "ImageDirectoryEntryToData",
    "ImageDirectoryEntryToDataEx",
    "ImageNtHeader",
    "ImageRvaToSection",
    "ImageRvaToVa",
    "ImagehlpApiVersion",
    "ImagehlpApiVersionEx",
    "MakeSureDirectoryPathExists",
    "MiniDumpReadDumpStream",
    "MiniDumpWriteDump",
    "RangeMapAddPeImageSections",
    "RangeMapCreate",
    "RangeMapFree",
    "RangeMapRead",
    "RangeMapRemove",
    "RangeMapWrite",
    "RemoveInvalidModuleList",
    "ReportSymbolLoadSummary",
    "SearchTreeForFile",
    "SearchTreeForFileW",
    "SetCheckUserInterruptShared",
    "SetSymLoadError",
    "StackWalk",
    "StackWalk64",
    "StackWalkEx",
    "SymAddSourceStream",
    "SymAddSourceStreamA",
    "SymAddSourceStreamW",
    "SymAddSymbol",
    "SymAddSymbolW",
    "SymAddrIncludeInlineTrace",
    "SymAllocDiaString",
    "SymCleanup",
    "SymCompareInlineTrace",
    "SymDeleteSymbol",
    "SymDeleteSymbolW",
    "SymEnumLines",
    "SymEnumLinesW",
    "SymEnumProcesses",
    "SymEnumSourceFileTokens",
    "SymEnumSourceFiles",
    "SymEnumSourceFilesW",
    "SymEnumSourceLines",
    "SymEnumSourceLinesW",
    "SymEnumSym",
    "SymEnumSymbols",
    "SymEnumSymbolsEx",
    "SymEnumSymbolsExW",
    "SymEnumSymbolsForAddr",
    "SymEnumSymbolsForAddrW",
    "SymEnumSymbolsW",
    "SymEnumTypes",
    "SymEnumTypesByName",
    "SymEnumTypesByNameW",
    "SymEnumTypesW",
    "SymEnumerateModules",
    "SymEnumerateModules64",
    "SymEnumerateModulesW64",
    "SymEnumerateSymbols",
    "SymEnumerateSymbols64",
    "SymEnumerateSymbolsW",
    "SymEnumerateSymbolsW64",
    "SymFindDebugInfoFile",
    "SymFindDebugInfoFileW",
    "SymFindExecutableImage",
    "SymFindExecutableImageW",
    "SymFindFileInPath",
    "SymFindFileInPathW",
    "SymFreeDiaString",
    "SymFromAddr",
    "SymFromAddrW",
    "SymFromIndex",
    "SymFromIndexW",
    "SymFromInlineContext",
    "SymFromInlineContextW",
    "SymFromName",
    "SymFromNameW",
    "SymFromToken",
    "SymFromTokenW",
    "SymFunctionTableAccess",
    "SymFunctionTableAccess64",
    "SymFunctionTableAccess64AccessRoutines",
    "SymGetDiaSession",
    "SymGetExtendedOption",
    "SymGetFileLineOffsets64",
    "SymGetHomeDirectory",
    "SymGetHomeDirectoryW",
    "SymGetLineFromAddr",
    "SymGetLineFromAddr64",
    "SymGetLineFromAddrEx",
    "SymGetLineFromAddrW64",
    "SymGetLineFromInlineContext",
    "SymGetLineFromInlineContextW",
    "SymGetLineFromName",
    "SymGetLineFromName64",
    "SymGetLineFromNameEx",
    "SymGetLineFromNameW64",
    "SymGetLineNext",
    "SymGetLineNext64",
    "SymGetLineNextEx",
    "SymGetLineNextW64",
    "SymGetLinePrev",
    "SymGetLinePrev64",
    "SymGetLinePrevEx",
    "SymGetLinePrevW64",
    "SymGetModuleBase",
    "SymGetModuleBase64",
    "SymGetModuleInfo",
    "SymGetModuleInfo64",
    "SymGetModuleInfoW",
    "SymGetModuleInfoW64",
    "SymGetOmapBlockBase",
    "SymGetOmaps",
    "SymGetOptions",
    "SymGetScope",
    "SymGetScopeW",
    "SymGetSearchPath",
    "SymGetSearchPathW",
    "SymGetSourceFile",
    "SymGetSourceFileChecksum",
    "SymGetSourceFileChecksumW",
    "SymGetSourceFileFromToken",
    "SymGetSourceFileFromTokenW",
    "SymGetSourceFileToken",
    "SymGetSourceFileTokenW",
    "SymGetSourceFileW",
    "SymGetSourceVarFromToken",
    "SymGetSourceVarFromTokenW",
    "SymGetSymFromAddr",
    "SymGetSymFromAddr64",
    "SymGetSymFromName",
    "SymGetSymFromName64",
    "SymGetSymNext",
    "SymGetSymNext64",
    "SymGetSymPrev",
    "SymGetSymPrev64",
    "SymGetSymbolFile",
    "SymGetSymbolFileW",
    "SymGetTypeFromName",
    "SymGetTypeFromNameW",
    "SymGetTypeInfo",
    "SymGetTypeInfoEx",
    "SymGetUnwindInfo",
    "SymInitialize",
    "SymInitializeW",
    "SymLoadModule",
    "SymLoadModule64",
    "SymLoadModuleEx",
    "SymLoadModuleExW",
    "SymMatchFileName",
    "SymMatchFileNameW",
    "SymMatchString",
    "SymMatchStringA",
    "SymMatchStringW",
    "SymNext",
    "SymNextW",
    "SymPrev",
    "SymPrevW",
    "SymQueryInlineTrace",
    "SymRefreshModuleList",
    "SymRegisterCallback",
    "SymRegisterCallback64",
    "SymRegisterCallbackW64",
    "SymRegisterFunctionEntryCallback",
    "SymRegisterFunctionEntryCallback64",
    "SymSearch",
    "SymSearchW",
    "SymSetContext",
    "SymSetDiaSession",
    "SymSetExtendedOption",
    "SymSetHomeDirectory",
    "SymSetHomeDirectoryW",
    "SymSetOptions",
    "SymSetParentWindow",
    "SymSetScopeFromAddr",
    "SymSetScopeFromIndex",
    "SymSetScopeFromInlineContext",
    "SymSetSearchPath",
    "SymSetSearchPathW",
    "SymSrvDeltaName",
    "SymSrvDeltaNameW",
    "SymSrvGetFileIndexInfo",
    "SymSrvGetFileIndexInfoW",
    "SymSrvGetFileIndexString",
    "SymSrvGetFileIndexStringW",
    "SymSrvGetFileIndexes",
    "SymSrvGetFileIndexesW",
    "SymSrvGetSupplement",
    "SymSrvGetSupplementW",
    "SymSrvIsStore",
    "SymSrvIsStoreW",
    "SymSrvStoreFile",
    "SymSrvStoreFileW",
    "SymSrvStoreSupplement",
    "SymSrvStoreSupplementW",
    "SymUnDName",
    "SymUnDName64",
    "SymUnloadModule",
    "SymUnloadModule64",
    "UnDecorateSymbolName",
    "UnDecorateSymbolNameW",
    "WinDbgExtensionDllInit",
    "_EFN_DumpImage",
    "block",
    "chksym",
    "dbghelp",
    "dh",
    "fptr",
    "homedir",
    "inlinedbg",
    "itoldyouso",
    "lmi",
    "lminfo",
    "omap",
    "optdbgdump",
    "optdbgdumpaddr",
    "srcfiles",
    "stack_force_ebp",
    "stackdbg",
    "sym",
    "symsrv",
    "vc7fpo",
};

static const WORD g_Ordinals[15] = {
    1101,
    1102,
    1103,
    1104,
    1105,
    1106,
    1107,
    1108,
    1109,
    1110,
    1115,
    1116,
    1117,
    1118,
    1119,
};

static void InitProxy() {
    static bool s_initialized = false;
    if (s_initialized) return;
    s_initialized = true;

    wchar_t sysPath[MAX_PATH];
    UINT len = GetSystemDirectoryW(sysPath, MAX_PATH);
    if (len > 0 && len < MAX_PATH - 13) {
        wcscat_s(sysPath, L"\\dbghelp.dll");
        HMODULE hReal = LoadLibraryW(sysPath);
        if (hReal) {
            for (int i = 0; i < 242; ++i) {
                g_pRealFuncs[i] = (void*)GetProcAddress(hReal, g_FuncNames[i]);
            }
            for (int i = 0; i < 15; ++i) {
                g_pRealFuncs[242 + i] = (void*)GetProcAddress(hReal, MAKEINTRESOURCEA(g_Ordinals[i]));
            }
        }
    }
}

namespace {
    std::string GetTargetWindowName() {
        char s[] = { 'N','o',' ','M','a','n','\'','s',' ','S','k','y', 0 };
        return std::string(s);
    }

    std::string GetModsSearchPattern() {
        char s[] = { '*','.','m','o','d','s', 0 };
        return std::string(s);
    }

    void LoadAllMods(HMODULE hModule) {
        char dllPath[MAX_PATH];
        GetModuleFileNameA(hModule, dllPath, MAX_PATH);

        std::string path(dllPath);
        size_t lastSlash = path.find_last_of("\\/");
        std::string dir = path.substr(0, lastSlash + 1);

        std::string searchPath = dir + GetModsSearchPattern();

        WIN32_FIND_DATAA findData;
        HANDLE hFind = FindFirstFileA(searchPath.c_str(), &findData);

        if (hFind != INVALID_HANDLE_VALUE) {
            do {
                if (!(findData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)) {
                    std::string fullModPath = dir + findData.cFileName;
                    LoadLibraryA(fullModPath.c_str());
                }
            } while (FindNextFileA(hFind, &findData));
            FindClose(hFind);
        }
    }

    HMODULE g_hProxy = nullptr;

    unsigned int __stdcall InitThread(void*) {
        const std::string winName = GetTargetWindowName();
        while (FindWindowA(nullptr, winName.c_str()) == nullptr) {
            Sleep(1000);
        }

        Sleep(5000);
        LoadAllMods(g_hProxy);
        return 0;
    }
}

BOOL APIENTRY DllMain(HMODULE hModule, DWORD reason, LPVOID lpReserved) {
    if (reason == DLL_PROCESS_ATTACH) {
        DisableThreadLibraryCalls(hModule);
        InitProxy();
        g_hProxy = hModule;
        _beginthreadex(nullptr, 0, InitThread, nullptr, 0, nullptr);
    }
    return TRUE;
}
