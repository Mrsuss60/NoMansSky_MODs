#pragma once
#include <windows.h>
#include <iostream>
#include <fstream>
#include <string>
#include <streambuf>
#include <mutex>
#include <ctime>

namespace Logger {

    class LogStreamBuf : public std::streambuf {
    public:
        LogStreamBuf() : m_consoleHandle(INVALID_HANDLE_VALUE), m_initialized(false) {}

        ~LogStreamBuf() {
            Shutdown();
        }

        bool Initialize(const std::string& logFilePath) {
            std::lock_guard<std::recursive_mutex> lock(m_mutex);
            if (m_initialized) return true;

            m_file.open(logFilePath, std::ios::out | std::ios::trunc);

            m_consoleHandle = CreateFileA("CONOUT$", GENERIC_WRITE, FILE_SHARE_WRITE, nullptr, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr);
            if (m_consoleHandle == INVALID_HANDLE_VALUE) {
                m_consoleHandle = GetStdHandle(STD_OUTPUT_HANDLE);
            }

            m_initialized = true;
            return true;
        }

        void Shutdown() {
            std::lock_guard<std::recursive_mutex> lock(m_mutex);
            if (!m_initialized) return;

            if (!m_lineBuffer.empty()) {
                OutputLine(m_lineBuffer);
                m_lineBuffer.clear();
            }

            if (m_consoleHandle != INVALID_HANDLE_VALUE && m_consoleHandle != GetStdHandle(STD_OUTPUT_HANDLE)) {
                CloseHandle(m_consoleHandle);
                m_consoleHandle = INVALID_HANDLE_VALUE;
            }

            if (m_file.is_open()) {
                m_file.flush();
                m_file.close();
            }

            m_initialized = false;
        }

    protected:
        int_type overflow(int_type c) override {
            if (c == traits_type::eof()) {
                return traits_type::not_eof(c);
            }

            std::lock_guard<std::recursive_mutex> lock(m_mutex);
            char ch = traits_type::to_char_type(c);
            ProcessChar(ch);
            return c;
        }

        std::streamsize xsputn(const char* s, std::streamsize count) override {
            std::lock_guard<std::recursive_mutex> lock(m_mutex);
            for (std::streamsize i = 0; i < count; ++i) {
                ProcessChar(s[i]);
            }
            return count;
        }

        int sync() override {
            std::lock_guard<std::recursive_mutex> lock(m_mutex);
            if (m_file.is_open()) {
                m_file.flush();
            }
            return 0;
        }

    private:
        HANDLE m_consoleHandle;
        std::ofstream m_file;
        std::string m_lineBuffer;
        std::recursive_mutex m_mutex;
        bool m_initialized;

        void WriteToConsole(const std::string& str) {
            if (m_consoleHandle == INVALID_HANDLE_VALUE) {
                m_consoleHandle = CreateFileA("CONOUT$", GENERIC_WRITE, FILE_SHARE_WRITE, nullptr, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr);
                if (m_consoleHandle == INVALID_HANDLE_VALUE) {
                    m_consoleHandle = GetStdHandle(STD_OUTPUT_HANDLE);
                }
            }
            if (m_consoleHandle != INVALID_HANDLE_VALUE) {
                DWORD written = 0;
                WriteConsoleA(m_consoleHandle, str.data(), static_cast<DWORD>(str.size()), &written, nullptr);
            }
        }

        void ProcessChar(char ch) {
            if (ch == '\r') return;
            if (ch == '\n') {
                OutputLine(m_lineBuffer);
                m_lineBuffer.clear();
            }
            else {
                m_lineBuffer.push_back(ch);
            }
        }

        void OutputLine(const std::string& line) {
            if (line.empty()) {
                if (m_file.is_open()) {
                    m_file << "\n";
                    m_file.flush();
                }
                WriteToConsole("\n");
                return;
            }

            size_t firstNonSpace = line.find_first_not_of(" \t\r\n");
            if (firstNonSpace != std::string::npos) {
                if (line[firstNonSpace] == '*') {
                    return;
                }
                if (line.find("Trying ") != std::string::npos ||
                    line.find("connect fail") != std::string::npos ||
                    line.find("Closing connection") != std::string::npos ||
                    line.find("Connected to ") != std::string::npos) {
                    return;
                }
            }

            bool hasTimestamp = false;
            if (line.size() >= 11 && line[0] == '[' && line[3] == ':' && line[6] == ':' && line[9] == ']' && line[10] == ' ') {
                hasTimestamp = true;
            }

            std::string timeStr;
            if (!hasTimestamp) {
                time_t now = time(nullptr);
                struct tm tmBuf;
                if (localtime_s(&tmBuf, &now) == 0) {
                    char buf[32];
                    strftime(buf, sizeof(buf), "[%H:%M:%S] ", &tmBuf);
                    timeStr = buf;
                }
            }

            if (m_file.is_open()) {
                if (!hasTimestamp) {
                    m_file << timeStr;
                }
                m_file << line << "\n";
                m_file.flush();
            }

            std::string out = (hasTimestamp ? "" : timeStr) + line + "\n";
            WriteToConsole(out);
        }
    };

    inline LogStreamBuf& GetLogStreamBuf() {
        static LogStreamBuf instance;
        return instance;
    }

    inline std::string GetLogFilePath() {
        char exePath[MAX_PATH];
        GetModuleFileNameA(nullptr, exePath, MAX_PATH);
        std::string path = exePath;
        size_t pos = path.find_last_of("\\/");
        if (pos != std::string::npos) path = path.substr(0, pos + 1);
        return path + "NoMansTimeLog.log";
    }

    class LoggerStream : public std::ostream {
    public:
        LoggerStream() : std::ostream(&GetLogStreamBuf()) {}
    };

    inline LoggerStream& Stream() {
        static LoggerStream s_stream;
        return s_stream;
    }

    inline bool Initialize() {
        return GetLogStreamBuf().Initialize(GetLogFilePath());
    }

    inline void Shutdown() {
        GetLogStreamBuf().Shutdown();
    }
}

#define LOG Logger::Stream()

