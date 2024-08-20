// TimeSimulator.cpp : Defines the functions for the static library.

#include "pch.h"
#include "TimeSimulator.h"

TimeSimulator::TimeSimulator()
    : isRunning(false), isReversed(false), accumulatedTime(std::chrono::milliseconds(0)) {}

void TimeSimulator::start() {
    std::lock_guard<std::mutex> lock(timeMutex);
    if (!isRunning) {
        isRunning = true;
        startTime = std::chrono::system_clock::now();
    }
}

void TimeSimulator::stop() {
    std::lock_guard<std::mutex> lock(timeMutex);
    if (isRunning) {
        isRunning = false;
        auto now = std::chrono::system_clock::now();
        accumulatedTime += std::chrono::duration_cast<std::chrono::milliseconds>(now - startTime);
    }
}

void TimeSimulator::reset() {
    std::lock_guard<std::mutex> lock(timeMutex);
    isRunning = false;
    isReversed = false;
    accumulatedTime = std::chrono::milliseconds(0);
}

void TimeSimulator::reverse() {
    std::lock_guard<std::mutex> lock(timeMutex);
    isReversed = !isReversed;
}

bool TimeSimulator::getIsReversed()
{
    return isReversed;
}

double TimeSimulator::getElapsedSeconds() const {
    std::lock_guard<std::mutex> lock(timeMutex);
    auto elapsed_ms = accumulatedTime;
    if (isRunning) {
        auto now = std::chrono::system_clock::now();
        auto duration = (isReversed ? startTime - now : now - startTime);
        elapsed_ms += std::chrono::duration_cast<std::chrono::milliseconds>(duration);
    }
    return std::chrono::duration<double>(elapsed_ms).count();
}

void TimeSimulator::update() {
    std::lock_guard<std::mutex> lock(timeMutex);
    if (isRunning) {
        auto now = std::chrono::system_clock::now();
        if (isReversed) {
            accumulatedTime -= std::chrono::duration_cast<std::chrono::milliseconds>(now - startTime);
        }
        else {
            accumulatedTime += std::chrono::duration_cast<std::chrono::milliseconds>(now - startTime);
        }
        startTime = now; // Update start time to current time
    }
}
