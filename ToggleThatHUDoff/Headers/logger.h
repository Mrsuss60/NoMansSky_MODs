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
            char ch = static_cast<char>(c);

            if (ch == '\n') {
                OutputLine(m_lineBuffer);
                m_lineBuffer.clear();
            } else if (ch != '\r') {
                m_lineBuffer += ch;
            }

            return c;
        }

        std::streamsize xsputn(const char* s, std::streamsize count) override {
            std::lock_guard<std::recursive_mutex> lock(m_mutex);
            for (std::streamsize i = 0; i < count; ++i) {
                char ch = s[i];
                if (ch == '\n') {
                    OutputLine(m_lineBuffer);
                    m_lineBuffer.clear();
                } else if (ch != '\r') {
                    m_lineBuffer += ch;
                }
            }
            return count;
        }

    private:
        void OutputLine(const std::string& line) {
            std::string formatted = line + "\n";

            if (m_consoleHandle != INVALID_HANDLE_VALUE) {
                DWORD written = 0;
                WriteFile(m_consoleHandle, formatted.data(), static_cast<DWORD>(formatted.size()), &written, nullptr);
            }

            if (m_file.is_open()) {
                m_file.write(formatted.data(), formatted.size());
                m_file.flush();
            }
        }

        std::recursive_mutex m_mutex;
        HANDLE m_consoleHandle;
        std::ofstream m_file;
        std::string m_lineBuffer;
        bool m_initialized;
    };

    class ScopedLog {
    public:
        ScopedLog(std::ostream& os, std::recursive_mutex& mutex)
            : m_os(os), m_lock(mutex) {}

        template<typename T>
        ScopedLog& operator<<(const T& val) {
            m_os << val;
            return *this;
        }

        ScopedLog& operator<<(std::ostream& (*manip)(std::ostream&)) {
            m_os << manip;
            return *this;
        }

    private:
        std::ostream& m_os;
        std::unique_lock<std::recursive_mutex> m_lock;
    };

    class LogManager {
    public:
        static LogManager& Instance() {
            static LogManager instance;
            return instance;
        }

        bool Initialize(const std::string& logFileName = "ToggleThatHUD.log") {
            char exePath[MAX_PATH];
            GetModuleFileNameA(nullptr, exePath, MAX_PATH);
            std::string path = exePath;
            size_t pos = path.find_last_of("\\/");
            if (pos != std::string::npos) {
                path = path.substr(0, pos + 1);
            }
            std::string fullPath = path + logFileName;

            if (m_buffer.Initialize(fullPath)) {
                m_stream.rdbuf(&m_buffer);
                return true;
            }
            return false;
        }

        void Shutdown() {
            m_buffer.Shutdown();
        }

        ScopedLog GetLog() {
            return ScopedLog(m_stream, m_mutex);
        }

    private:
        LogManager() : m_stream(nullptr) {}
        ~LogManager() { Shutdown(); }

        LogStreamBuf m_buffer;
        std::ostream m_stream;
        std::recursive_mutex m_mutex;
    };

    inline bool Initialize(const std::string& logFileName = "ToggleThatHUD.log") {
        return LogManager::Instance().Initialize(logFileName);
    }

    inline void Shutdown() {
        LogManager::Instance().Shutdown();
    }

    inline ScopedLog Stream() {
        return LogManager::Instance().GetLog();
    }
}

#define LOG Logger::Stream()
