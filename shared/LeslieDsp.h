// MDA Leslie DSP, derived from mdaLeslieProcessor.cpp.
// Copyright (c) 2008 Paul Kellett. VST3 adaptation by Arne Scheffler.
// MIT licence: see licenses/MDA-Leslie-MIT.txt.
#pragma once

#include <algorithm>
#include <array>
#include <cmath>
#include <vector>

namespace mda {
class LeslieDsp final {
public:
    void prepare(double sampleRate)
    {
        sampleRate_ = std::isfinite(sampleRate) && sampleRate > 0.0 ? sampleRate : 44100.0;
        // The original 201-sample ring is too short for maximum depth above
        // 76 kHz. Allocate once, outside the audio callback, for the full delay.
        delay_.resize(static_cast<std::size_t>(std::max(201.0, std::ceil(2.0 * sampleRate_ / 760.0) + 4.0)));
        reset();
        setParameters(parameters_);
    }

    void reset() noexcept
    {
        std::fill(delay_.begin(), delay_.end(), 0.0f);
        position_ = 0;
        filter1_ = filter2_ = lowSpeed_ = highSpeed_ = lowPhase_ = 0.0f;
        highPhase_ = 1.6f;
    }

    void setParameters(const std::array<float, 9>& parameters) noexcept
    {
        for (std::size_t i = 0; i < parameters.size(); ++i)
            parameters_[i] = std::isfinite(parameters[i]) ? std::clamp(parameters[i], 0.0f, 1.0f) : 0.5f;
        const auto& p = parameters_;
        const float inverseRate = static_cast<float>(1.0 / sampleRate_);
        const float speed = twoPi * inverseRate * 2.0f * p[8];
        crossover_ = 1.0f - std::pow(10.0f, p[6] * (2.27f - 0.54f * p[6]) - 1.92f);
        float lowMomentumSeconds = 0.12f, highMomentumSeconds = 0.10f;
        switch (static_cast<int>(p[0] * 2.0f)) {
            case 0: lowTarget_ = highTarget_ = 0.0f; break;
            case 1:
                lowTarget_ = 0.49f; highTarget_ = 0.66f;
                lowMomentumSeconds = 0.27f; highMomentumSeconds = 0.18f;
                break;
            default:
                lowTarget_ = 5.31f; highTarget_ = 6.40f;
                lowMomentumSeconds = 0.14f; highMomentumSeconds = 0.09f;
                break;
        }
        lowMomentum_ = std::pow(10.0f, -inverseRate / lowMomentumSeconds);
        highMomentum_ = std::pow(10.0f, -inverseRate / highMomentumSeconds);
        lowTarget_ *= speed;
        highTarget_ *= speed;
        gain_ = 0.4f * std::pow(10.0f, 2.0f * p[7] - 1.0f);
        lowWidth_ = p[1] * p[1];
        lowLevel_ = gain_ * 0.9f * p[2] * p[2];
        highWidth_ = p[3] * p[3];
        highDepth_ = p[4] * p[4] * static_cast<float>(sampleRate_) / 760.0f;
        highLevel_ = gain_ * 0.9f * p[5] * p[5];
    }

    void process(float* left, float* right, int samples) noexcept
    {
        if (delay_.empty() || samples <= 0) return;
        constexpr float step = 32.0f, inverseStep = 1.0f / step;
        auto lowSpeed = lowSpeed_, highSpeed = highSpeed_;
        auto lowPhase = lowPhase_, highPhase = highPhase_;
        auto f1 = filter1_, f2 = filter2_;
        const float highTarget = highTarget_ * (1.0f - highMomentum_);
        const float lowTarget = lowTarget_ * (1.0f - lowMomentum_);
        float highCos = std::cos(highPhase);
        highCos *= highCos * highCos;
        float lowCos = std::cos(lowPhase), highSin = std::sin(highPhase), lowSin = std::sin(lowPhase);
        float dHighCos = 0.0f, dLowCos = 0.0f, dHighSin = 0.0f, dLowSin = 0.0f;
        int remaining = 0;
        const auto ringSize = static_cast<int>(delay_.size());
        for (int i = 0; i < samples; ++i) {
            const float input = left[i] + right[i];
            if (remaining > 0) --remaining;
            else {
                lowSpeed = lowMomentum_ * lowSpeed + lowTarget;
                highSpeed = highMomentum_ * highSpeed + highTarget;
                lowPhase += step * lowSpeed;
                highPhase += step * highSpeed;
                const float nextCos = std::cos(highPhase + step * highSpeed);
                dHighCos = inverseStep * (nextCos * nextCos * nextCos - highCos);
                dLowCos = inverseStep * (std::cos(lowPhase + step * lowSpeed) - lowCos);
                dHighSin = inverseStep * (std::sin(highPhase + step * highSpeed) - highSin);
                dLowSin = inverseStep * (std::sin(lowPhase + step * lowSpeed) - lowSin);
                remaining = 32;
            }
            f1 = crossover_ * (f1 - input) + input;
            f2 = crossover_ * (f2 - f1) + f1;
            float high = (gain_ - highLevel_ * highCos) * (input - f2);
            float low = (gain_ - lowLevel_ * lowCos) * f2;
            position_ = position_ > 0 ? position_ - 1 : ringSize - 1;
            const float readPosition = static_cast<float>(position_) + highDepth_ * (1.0f + highCos);
            const auto whole = static_cast<int>(std::floor(readPosition));
            const float fraction = readPosition - static_cast<float>(whole);
            // Full modulo also handles multiple wraps and tiny negative LFO
            // interpolation errors, unlike the original single subtraction.
            const int a = (whole % ringSize + ringSize) % ringSize;
            const int b = (a + 1) % ringSize;
            delay_[static_cast<std::size_t>(position_)] = high;
            const float delayed = delay_[static_cast<std::size_t>(a)];
            high += delayed + fraction * (delay_[static_cast<std::size_t>(b)] - delayed);
            const float sum = low + high;
            high *= highWidth_ * highSin;
            low *= lowWidth_ * lowSin;
            left[i] = sum + high - low;
            right[i] = sum + low - high;
            highCos += dHighCos; lowCos += dLowCos;
            highSin += dHighSin; lowSin += dLowSin;
        }
        lowSpeed_ = lowSpeed; highSpeed_ = highSpeed;
        lowPhase_ = std::fmod(lowPhase + (step - static_cast<float>(remaining)) * lowSpeed, twoPi);
        highPhase_ = std::fmod(highPhase + (step - static_cast<float>(remaining)) * highSpeed, twoPi);
        filter1_ = std::abs(f1) > 1.0e-10f ? f1 : 0.0f;
        filter2_ = std::abs(f2) > 1.0e-10f ? f2 : 0.0f;
    }

private:
    static constexpr float twoPi = 6.2831853f;
    double sampleRate_ = 44100.0;
    std::array<float, 9> parameters_{.66f, .5f, .5f, .7f, .6f, .7f, .48f, .5f, .6f};
    std::vector<float> delay_;
    int position_ = 0;
    float filter1_ = 0.0f, filter2_ = 0.0f;
    float lowSpeed_ = 0.0f, highSpeed_ = 0.0f, lowPhase_ = 0.0f, highPhase_ = 1.6f;
    float crossover_ = 0.0f, gain_ = 0.0f;
    float lowTarget_ = 0.0f, highTarget_ = 0.0f;
    float lowMomentum_ = 0.0f, highMomentum_ = 0.0f;
    float lowWidth_ = 0.0f, lowLevel_ = 0.0f;
    float highWidth_ = 0.0f, highLevel_ = 0.0f, highDepth_ = 0.0f;
};
}
