/*
 * SideQuest Starting Point
 *
 * Basically lets paul bootstrap his projects.
 *
 * Copyright 2024-2025, Paul Walker and Various authors, as described in the github
 * transaction log.
 *
 * This source repo is released under the MIT license, but has
 * GPL3 dependencies, as such the combined work will be
 * released under GPL3.
 *
 * The source code and license are at https://github.com/baconpaul/sidequest-startingpoint
 */

#ifndef BACONPAUL_SIDEQUEST_ENGINE_ASCIISCOPE_AUDIO_SNAPSHOT_H
#define BACONPAUL_SIDEQUEST_ENGINE_ASCIISCOPE_AUDIO_SNAPSHOT_H

#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>
#include <mutex>

namespace baconpaul::sidequest_ns
{
struct AsciiscopeAudioSnapshot
{
    static constexpr uint32_t maxSamples{128};

    std::array<float, maxSamples> left{};
    std::array<float, maxSamples> right{};
    uint32_t sampleCount{0};
    uint64_t frameIndex{0};
    float leftPeak{0.0f};
    float rightPeak{0.0f};
    float leftRms{0.0f};
    float rightRms{0.0f};
    float stereoCorrelation{0.0f};
    float transientAmount{0.0f};
};

struct AsciiscopeAudioSnapshotBuffer
{
    void publish(const float *left, const float *right, uint32_t sampleCount)
    {
        if (!mutex.try_lock())
            return;
        std::lock_guard<std::mutex> lock(mutex, std::adopt_lock);

        latest = {};
        latest.frameIndex = ++frameCounter;
        latest.sampleCount = std::min(sampleCount, AsciiscopeAudioSnapshot::maxSamples);

        float leftSquares{0.0f};
        float rightSquares{0.0f};
        float crossSum{0.0f};
        for (uint32_t i = 0; i < latest.sampleCount; ++i)
        {
            const auto l = left ? left[i] : 0.0f;
            const auto r = right ? right[i] : l;
            latest.left[i] = l;
            latest.right[i] = r;
            latest.leftPeak = std::max(latest.leftPeak, std::abs(l));
            latest.rightPeak = std::max(latest.rightPeak, std::abs(r));
            leftSquares += l * l;
            rightSquares += r * r;
            crossSum += l * r;
        }

        if (latest.sampleCount > 0)
        {
            const auto denom = static_cast<float>(latest.sampleCount);
            latest.leftRms = std::sqrt(leftSquares / denom);
            latest.rightRms = std::sqrt(rightSquares / denom);
            const auto correlationDenom = std::sqrt(leftSquares * rightSquares);
            if (correlationDenom > 0.000001f)
                latest.stereoCorrelation = std::clamp(crossSum / correlationDenom, -1.0f, 1.0f);

            const auto peak = std::max(latest.leftPeak, latest.rightPeak);
            const auto rms = (latest.leftRms + latest.rightRms) * 0.5f;
            if (rms > 0.000001f)
                latest.transientAmount = std::clamp((peak / rms - 1.0f) * 0.35f, 0.0f, 1.0f);
        }
    }

    bool pullLatest(AsciiscopeAudioSnapshot &target, uint64_t &lastFrameSeen)
    {
        std::lock_guard<std::mutex> lock(mutex);
        if (latest.frameIndex == 0 || latest.frameIndex == lastFrameSeen)
            return false;

        target = latest;
        lastFrameSeen = latest.frameIndex;
        return true;
    }

  private:
    std::mutex mutex;
    AsciiscopeAudioSnapshot latest;
    uint64_t frameCounter{0};
};
} // namespace baconpaul::sidequest_ns

#endif // BACONPAUL_SIDEQUEST_ENGINE_ASCIISCOPE_AUDIO_SNAPSHOT_H
