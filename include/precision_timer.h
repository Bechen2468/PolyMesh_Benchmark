#ifndef PRECISION_TIMER_H
#define PRECISION_TIMER_H
// ----------------------------------------------------------------------------
#include <chrono>
// ----------------------------------------------------------------------------


class Precision_Timer {
private:
    std::chrono::high_resolution_clock::time_point _timestamp_start;
    std::chrono::high_resolution_clock::time_point _timestamp_stop;

public:
    Precision_Timer();
    void start();
    void stop();
    long long result();
};

#endif