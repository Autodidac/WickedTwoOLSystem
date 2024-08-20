#include "Logger.h"

Logger::Logger(OutputMode mode, const std::string& filename)
    : mode_(mode) {
    if (mode_ == OutputMode::File) {
        file_.open(filename, std::ios::app);
        if (!file_.is_open()) {
            throw std::runtime_error("Failed to open log file.");
        }
    }
}

Logger::~Logger() {
    if (mode_ == OutputMode::File && file_.is_open()) {
        file_.close();
    }
}

void Logger::Log(LogLevel level, const std::string& message) {
    std::lock_guard<std::mutex> lock(mutex_);

    std::string logMessage = GetCurrentTime() + " [" + LogLevelToString(level) + "] " + message;

    if (mode_ == OutputMode::Console) {
        std::cout << logMessage << std::endl;
    }
    else if (mode_ == OutputMode::File && file_.is_open()) {
        file_ << logMessage << std::endl;
    }
}

std::string Logger::GetCurrentTime() const {
    auto now = std::chrono::system_clock::now();
    auto time = std::chrono::system_clock::to_time_t(now);

    struct tm timeinfo;
    localtime_s(&timeinfo, &time);

    std::stringstream ss;
    ss << std::put_time(&timeinfo, "%Y-%m-%d %H:%M:%S");
    return ss.str();
}

std::string Logger::LogLevelToString(LogLevel level) const {
    switch (level) {
    case LogLevel::Info: return "INFO";
    case LogLevel::Warning: return "WARNING";
    case LogLevel::Error: return "ERROR";
    default: return "UNKNOWN";
    }
}
