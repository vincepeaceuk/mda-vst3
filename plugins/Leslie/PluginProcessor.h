// SPDX-License-Identifier: AGPL-3.0-or-later
#pragma once
#include <JuceHeader.h>
#include "LeslieDsp.h"
#include <array>
#include <atomic>

class MDALeslieAudioProcessor final : public juce::AudioProcessor {
public:
    MDALeslieAudioProcessor();
    const juce::String getName() const override { return JucePlugin_Name; }
    void prepareToPlay(double rate, int) override { dsp_.prepare(rate); }
    void releaseResources() override {}
    void reset() override { dsp_.reset(); }
    bool isBusesLayoutSupported(const BusesLayout&) const override;
    void processBlock(juce::AudioBuffer<float>&, juce::MidiBuffer&) override;
    bool hasEditor() const override { return true; }
    juce::AudioProcessorEditor* createEditor() override;
    bool acceptsMidi() const override { return false; }
    bool producesMidi() const override { return false; }
    bool isMidiEffect() const override { return false; }
    double getTailLengthSeconds() const override { return 0.1; }
    int getNumPrograms() override { return 3; }
    int getCurrentProgram() override { return program_.load(std::memory_order_relaxed); }
    void setCurrentProgram(int) override;
    const juce::String getProgramName(int) override;
    void changeProgramName(int, const juce::String&) override {}
    void getStateInformation(juce::MemoryBlock&) override;
    void setStateInformation(const void*, int) override;
    juce::AudioProcessorValueTreeState apvts{*this, nullptr, "Parameters", layout()};

private:
    static juce::AudioProcessorValueTreeState::ParameterLayout layout();
    mda::LeslieDsp dsp_;
    std::atomic<int> program_{0};
    std::array<std::atomic<float>*, 9> values_{};
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(MDALeslieAudioProcessor)
};
