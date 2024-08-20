#pragma once

#include <string>
#include <fstream>
#include <iostream>
#include <mutex>
#include <chrono>
#include <iomanip>
#include <sstream>

class Logger {
public:
	enum class LogLevel {
		Info,
		Warning,
		Error
		// You can add more levels like Debug, Trace here
	};

	enum class OutputMode {
		Console,
		File
	};

	// Constructor to initialize the logger with output mode and filename
	Logger(OutputMode mode, const std::string& filename = "log.txt");
	~Logger();

	// Log a message with a specific log level
	void Log(LogLevel level, const std::string& message);

private:
	// Get current time in string format
	std::string GetCurrentTime() const;

	// Convert log level enum to string
	std::string LogLevelToString(LogLevel level) const;

	OutputMode mode_;  // Output mode (Console or File)
	std::ofstream file_;  // File stream for logging to a file
	std::mutex mutex_;  // Mutex for thread safety
};
