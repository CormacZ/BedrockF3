// src/f3_debug/util/FpsCounter.cpp

#include "f3_debug/util/FpsCounter.h"

#include <algorithm>
#include <numeric>

namespace f3_debug::util {

FpsCounter::FpsCounter(std::size_t windowSize) : mWindowSize(windowSize == 0 ? 1 : windowSize) {}

double FpsCounter::tick(double deltaMs) noexcept {
    mLastDeltaMs = deltaMs;
    if (deltaMs <= 0.0) {
        return 0.0;
    }
    mSamples.push_back(deltaMs);
    if (mSamples.size() > mWindowSize) {
        mSamples.pop_front();
    }
    const double sum = std::accumulate(mSamples.begin(), mSamples.end(), 0.0);
    return 1000.0 / (sum / static_cast<double>(mSamples.size()));
}

void FpsCounter::clear() noexcept {
    mSamples.clear();
    mLastDeltaMs = 0.0;
}

} // namespace f3_debug::util
