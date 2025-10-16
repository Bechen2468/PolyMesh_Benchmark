#include "precision_timer.h"


Precision_Timer::Precision_Timer() {

}


void Precision_Timer::start() {
    _timestamp_start = std::chrono::high_resolution_clock::now();
}


void Precision_Timer::stop() {
    _timestamp_stop = std::chrono::high_resolution_clock::now();
}


long long Precision_Timer::result() {
    return std::chrono::duration_cast<std::chrono::microseconds>(_timestamp_stop - _timestamp_start).count();
}

