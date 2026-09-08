#include "config_file.h"
#include "globals.h"
#include <windows.h>
#include <fstream>
#include <string>
#include <sys/stat.h>
#include <cmath>
#include <algorithm>

static time_t g_lastConfigModTime = 0;

static std::string GetConfigPath() {
    char exePath[MAX_PATH];
    GetModuleFileNameA(nullptr, exePath, MAX_PATH);
    std::string path = exePath;
    size_t pos = path.find_last_of("\\/");
    if (pos != std::string::npos) path = path.substr(0, pos + 1);
    return path + "NoMansTimeConfig.ini";
}

bool HasConfigChanged() {
    struct stat fileStat;
    if (stat(GetConfigPath().c_str(), &fileStat) == 0) return fileStat.st_mtime != g_lastConfigModTime;
    return false;
}

void SaveConfig() {
    std::string path = GetConfigPath();
    std::ofstream f(path);
    if (!f.is_open()) return;

    f << "[Settings]\n";
    f << "; Day/Night cycle speed multiplier (0.0 to 30.0)\n";
    f << "DayNightCycleSpeed=" << Config::TimeSpeedMultiplier.load() << "\n";
    f << "; Step size in minutes per hotkey press (1 to 60)\n";
    f << "StepMinutes=" << Config::StepMinutes.load() << "\n";
    f << "; Freeze time progression\n";
    f << "FreezeTime=" << (Config::FreezeTime.load() ? "true" : "false") << "\n";
    f << "; Enable hotkey controls\n";
    f << "EnableHotkeys=" << (Config::EnableHotkeys.load() ? "true" : "false") << "\n\n";

    f << "[Hotkeys]\n";
    f << std::hex << std::showbase << std::uppercase;
    f << "; Advance time (default: Numpad + / 0x6B)\n";
    f << "AdvanceTimeKey=" << Config::AdvanceTimeKey << "\n";
    f << "; Rewind time (default: Numpad - / 0x6D)\n";
    f << "RewindTimeKey=" << Config::RewindTimeKey << "\n\n";
    f << std::dec << std::noshowbase << std::nouppercase;

    f << "; Virtualkey codes can be used for setting up custom hotkeys.\n";
    f << "# https://learn.microsoft.com/en-us/windows/win32/inputdev/virtual-key-codes\n";
    f << "# --- Mouse ---\n";
    f << "# VK_LBUTTON    0x01 (Left Mouse Button)\n";
    f << "# VK_RBUTTON    0x02 (Right Mouse Button)\n";
    f << "# VK_MBUTTON    0x04 (Middle Mouse Button)\n";
    f << "# VK_XBUTTON1   0x05 (Side Mouse Button1)\n";
    f << "# VK_XBUTTON2   0x06 (Side Mouse Button2)\n";
    f << "# --- Standard Keys ---\n";
    f << "# VK_BACK       0x08 (Backspace)\n";
    f << "# VK_TAB        0x09 (Tab)\n";
    f << "# VK_RETURN     0x0D (Enter)\n";
    f << "# VK_SHIFT      0x10 (Shift)\n";
    f << "# VK_CONTROL    0x11 (Ctrl)\n";
    f << "# VK_MENU       0x12 (Alt)\n";
    f << "# VK_PAUSE      0x13 (Pause)\n";
    f << "# VK_CAPITAL    0x14 (Caps Lock)\n";
    f << "# VK_ESCAPE     0x1B (Esc)\n";
    f << "# VK_SPACE      0x20 (Spacebar)\n";
    f << "# --- Navigation ---\n";
    f << "# VK_PRIOR      0x21 (Page Up)\n";
    f << "# VK_NEXT       0x22 (Page Down)\n";
    f << "# VK_END        0x23 (End)\n";
    f << "# VK_HOME       0x24 (Home)\n";
    f << "# VK_LEFT       0x25 (Left Arrow)\n";
    f << "# VK_UP         0x26 (Up Arrow)\n";
    f << "# VK_RIGHT      0x27 (Right Arrow)\n";
    f << "# VK_DOWN       0x28 (Down Arrow)\n";
    f << "# VK_INSERT     0x2D (Insert)\n";
    f << "# VK_DELETE     0x2E (Delete)\n";
    f << "# VK_SNAPSHOT   0x2C (Print Screen)\n";
    f << "# --- Numbers (0-9) ---\n";
    f << "# 0=0x30, 1=0x31, 2=0x32, 3=0x33, 4=0x34\n";
    f << "# 5=0x35, 6=0x36, 7=0x37, 8=0x38, 9=0x39\n";
    f << "# --- Alphabet (A-Z) ---\n";
    f << "# A=0x41, B=0x42, C=0x43, D=0x44, E=0x45, F=0x46\n";
    f << "# G=0x47, H=0x48, I=0x49, J=0x4A, K=0x4B, L=0x4C\n";
    f << "# M=0x4D, N=0x4E, O=0x4F, P=0x50, Q=0x51, R=0x52\n";
    f << "# S=0x53, T=0x54, U=0x55, V=0x56, W=0x57, X=0x58\n";
    f << "# Y=0x59, Z=0x5A\n";
    f << "# --- Numpad ---\n";
    f << "# VK_NUMPAD0    0x60 (Numpad 0)\n";
    f << "# VK_NUMPAD1    0x61 (Numpad 1)\n";
    f << "# VK_NUMPAD2    0x62 (Numpad 2)\n";
    f << "# VK_NUMPAD3    0x63 (Numpad 3)\n";
    f << "# VK_NUMPAD4    0x64 (Numpad 4)\n";
    f << "# VK_NUMPAD5    0x65 (Numpad 5)\n";
    f << "# VK_NUMPAD6    0x66 (Numpad 6)\n";
    f << "# VK_NUMPAD7    0x67 (Numpad 7)\n";
    f << "# VK_NUMPAD8    0x68 (Numpad 8)\n";
    f << "# VK_NUMPAD9    0x69 (Numpad 9)\n";
    f << "# VK_MULTIPLY   0x6A (Numpad *)\n";
    f << "# VK_ADD        0x6B (Numpad +)\n";
    f << "# VK_SEPARATOR  0x6C (Numpad Enter/Separator)\n";
    f << "# VK_SUBTRACT   0x6D (Numpad -)\n";
    f << "# VK_DECIMAL    0x6E (Numpad .)\n";
    f << "# VK_DIVIDE     0x6F (Numpad /)\n";
    f << "# --- F-Keys ---\n";
    f << "# VK_F1         0x70\n";
    f << "# VK_F2         0x71\n";
    f << "# VK_F3         0x72\n";
    f << "# VK_F4         0x73\n";
    f << "# VK_F5         0x74\n";
    f << "# VK_F6         0x75\n";
    f << "# VK_F7         0x76\n";
    f << "# VK_F8         0x77\n";
    f << "# VK_F9         0x78\n";
    f << "# VK_F10        0x79\n";
    f << "# VK_F11        0x7A\n";
    f << "# VK_F12        0x7B\n";

    f.close();

    struct stat fileStat;
    if (stat(path.c_str(), &fileStat) == 0) g_lastConfigModTime = fileStat.st_mtime;
}

void LoadConfig() {
    std::string path = GetConfigPath();
    std::ifstream f(path);
    if (!f.is_open()) {
        SaveConfig();
        return;
    }

    std::string line;
    while (std::getline(f, line)) {
        if (line.empty() || line[0] == '#' || line[0] == '[' || line[0] == ';') continue;
        size_t eq = line.find('=');
        if (eq == std::string::npos) continue;

        std::string key = line.substr(0, eq), val = line.substr(eq + 1);
        key.erase(0, key.find_first_not_of(" \t\r\n")); key.erase(key.find_last_not_of(" \t\r\n") + 1);
        val.erase(0, val.find_first_not_of(" \t\r\n")); val.erase(val.find_last_not_of(" \t\r\n") + 1);

        try {
            if (key == "DayNightCycleSpeed") {
                float speed = std::stof(val);
                speed = (std::max)(0.0f, (std::min)(30.0f, speed));
                Config::TimeSpeedMultiplier.store(speed);
                if (speed <= 0.0001f) {
                    Config::FreezeTime.store(true);
                }
            }
            else if (key == "StepMinutes") {
                int step = std::stoi(val);
                step = (std::max)(1, (std::min)(60, step));
                Config::StepMinutes.store(step);
            }
            else if (key == "FreezeTime") {
                Config::FreezeTime.store(val == "true" || val == "1");
            }
            else if (key == "EnableHotkeys") {
                Config::EnableHotkeys.store(val == "true" || val == "1");
            }
            else if (key == "AdvanceTimeKey") {
                Config::AdvanceTimeKey = std::stoi(val, nullptr, 16);
            }
            else if (key == "RewindTimeKey") {
                Config::RewindTimeKey = std::stoi(val, nullptr, 16);
            }
        }
        catch (...) {}
    }
    f.close();

    struct stat fileStat;
    if (stat(path.c_str(), &fileStat) == 0) g_lastConfigModTime = fileStat.st_mtime;
}
