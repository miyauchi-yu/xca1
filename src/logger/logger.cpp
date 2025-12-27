#include "logger.hpp"
#include <iostream>
#include <chrono>
#include <iomanip>
#include <ctime>

namespace fs = std::filesystem;

Logger::Logger() {
    // ディレクトリがなければ作成
    if (!fs::exists(logDir_)) {
        fs::create_directory(logDir_);
    }

    // 追記モードでオープン
    file_.open(fs::path(logDir_) / logFileName_, std::ios::app);
}

Logger::~Logger() {
    file_.close();
}

Logger& Logger::getInstance() {
    static Logger instance;
    return instance;
}

void Logger::log(LogLevel level, const std::string& message) {
    std::lock_guard<std::mutex> lock(mutex_);
    rotateLogsIfNeeded();

    std::string line = "[" + timestamp() + "][" + levelToString(level) + "] " + message;

    // ファイル
    if (file_.is_open()) {
        file_ << line << std::endl;
    }
}

void Logger::rotateLogsIfNeeded() {
    if (!file_.is_open()) return;

    auto logPath = fs::path(logDir_) / logFileName_;
    if (!fs::exists(logPath)) return;

    auto size = fs::file_size(logPath);
    if (size < maxFileSize_) return;

    file_.close();

    // 古いログを削除またはリネーム
    for (int i = maxBackupCount_ - 1; i >= 1; --i) {
        auto older = fs::path(logDir_) / ("app.log." + std::to_string(i));
        auto newer = fs::path(logDir_) / ("app.log." + std::to_string(i + 1));
        if (fs::exists(older)) {
            if (i == maxBackupCount_ - 1 && fs::exists(newer)) {
                fs::remove(newer); // 最古のログ削除
            }
            fs::rename(older, newer);
        }
    }

    // 現在のログを app.log.1 に
    auto rotated = fs::path(logDir_) / "app.log.1";
    fs::rename(logPath, rotated);

    // 新しいファイルを作成
    file_.open(logPath, std::ios::trunc);
    log(LogLevel::Info, "Log rotated");
}

std::string Logger::timestamp() const {
    auto now = std::chrono::system_clock::now();
    auto t = std::chrono::system_clock::to_time_t(now);
    std::tm localTime{};
#ifdef _WIN32
    localtime_s(&localTime, &t);
#else
    localtime_r(&t, &localTime);
#endif
    std::ostringstream oss;
    oss << std::put_time(&localTime, "%Y/%m/%d %H:%M:%S");
    return oss.str();
}

std::string Logger::levelToString(LogLevel level) const {
    switch (level) {
        case LogLevel::Info:
            return "INFO";
        case LogLevel::Warning:
            return "WARN";
        case LogLevel::Error:
            return "ERROR";
        default:
            return "UNKNOWN";
    }
}
