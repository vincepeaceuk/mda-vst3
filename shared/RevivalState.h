// SPDX-License-Identifier: AGPL-3.0-or-later
#pragma once
#include <JuceHeader.h>
#include <atomic>
#include <cmath>

namespace mda {
inline void saveState(juce::AudioProcessorValueTreeState& parameters, int program, juce::MemoryBlock& data)
{
    auto state = parameters.copyState();
    state.setProperty("programIndex", program, nullptr);
    state.setProperty("revivalVersion", 1, nullptr);
    juce::AudioProcessor::copyXmlToBinary(*state.createXml(), data);
}

inline void restoreState(juce::AudioProcessorValueTreeState& parameters, const void* data,
                         int size, std::atomic<int>& program, int programCount)
{
    if (data == nullptr || size <= 0 || size > 2 * 1024 * 1024) return;
    const auto xml = juce::AudioProcessor::getXmlFromBinary(data, size);
    if (xml == nullptr || !xml->hasTagName(parameters.state.getType())) return;
    auto state = juce::ValueTree::fromXml(*xml);
    if (!state.isValid()) return;
    for (auto child : state) {
        auto* parameter = parameters.getParameter(child.getProperty("id").toString());
        const float value = child.getProperty("value", 0.0f);
        if (parameter == nullptr || !std::isfinite(value)) return;
    }
    const int index = juce::jlimit(0, juce::jmax(0, programCount - 1), static_cast<int>(state.getProperty("programIndex", 0)));
    parameters.replaceState(state);
    // Loading a factory program here would overwrite the user's edited values.
    program.store(index, std::memory_order_relaxed);
}
}
