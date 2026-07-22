// src/f3_debug/util/FpsCounter.h
// 1-second moving average of frame time, exposed as FPS.

#pragma once

#include <chrono>
#include <cstddef>
#include <deque>

namespace f3_debug::util {

class FpsCounter {
public:
    explicit FpsCounter(std::size_t windowSize = 60);

    // Record a frame of `deltaMs` milliseconds and return the smoothed FPS.
    double tick(double deltaMs) noexcept;

    [[nodiscard]] double lastDeltaMs() const noexcept { return mLastDeltaMs; }
    [[nodiscard]] std::size_t sampleCount() const noexcept { return mSamples.size(); }

    void clear() noexcept;

private:
    std::size_t mWindowSize;
    std::deque<double> mSamples;
    double mLastDeltaMs = 0.0;
};

} // namespace f3_debug::util
