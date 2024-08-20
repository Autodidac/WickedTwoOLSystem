#pragma once
#include "framework.h"

#include <chrono>
#include <mutex>

class TimeSimulator {
public:
    TimeSimulator();

    void start();                           // Start the time simulation
    void stop();                            // Stop the time simulation
    void reset();                           // Reset the time simulation
    void reverse();                         // Reverse the time simulation
    bool getIsReversed();

    double getElapsedSeconds() const;       // Get elapsed time in seconds
    void update();                          // Update the simulation time

private:
    bool isRunning;
    bool isReversed;
    std::chrono::milliseconds accumulatedTime;
    std::chrono::system_clock::time_point startTime;
    mutable std::mutex timeMutex;           // Mutex to synchronize time-related operations
};
