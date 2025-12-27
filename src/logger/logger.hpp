#pragma once
#include <string>
#include <fstream>
#include <mutex>
#include <filesystem>

enum class LogLevel {
    Info,
    Warning,
    Error
};

class Logger {
    public:
        static Logger& getInstance();
        void log(LogLevel level, const std::string& message);
    private:
        Logger();
        ~Logger();
        void rotateLogsIfNeeded();
        std::string timestamp() const;
        std::string levelToString(LogLevel level) const;
        std::ofstream file_;
        std::mutex mutex_;
        const std::string logDir_ = "logs";
        const std::string logFileName_ = "app.log";
        const size_t maxFileSize_ = 1 * 1024 * 1024; // 1MB
        const int maxBackupCount_ = 5;
};
