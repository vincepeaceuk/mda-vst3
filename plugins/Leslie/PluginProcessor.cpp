// SPDX-License-Identifier: AGPL-3.0-or-later
// Original rotary algorithm: Paul Kellett, MIT; see shared/LeslieDsp.h.
#include "PluginProcessor.h"
#include "RevivalEditor.h"
#include "RevivalState.h"

namespace {
constexpr std::array<const char*, 9> ids{"Speed Mode", "Lo Width", "Lo Throb", "Hi Width", "Hi Depth", "Hi Throb", "X-Over", "Output", "Speed"};
}

MDALeslieAudioProcessor::MDALeslieAudioProcessor()
    : AudioProcessor(BusesProperties().withInput("Input", juce::AudioChannelSet::stereo(), true)
                                      .withOutput("Output", juce::AudioChannelSet::stereo(), true))
{
    for (std::size_t i = 0; i < ids.size(); ++i) values_[i] = apvts.getRawParameterValue(ids[i]);
}

bool MDALeslieAudioProcessor::isBusesLayoutSupported(const BusesLayout& buses) const
{
    return buses.getMainOutputChannelSet() == juce::AudioChannelSet::stereo()
        && (buses.getMainInputChannelSet() == juce::AudioChannelSet::stereo()
            || buses.getMainInputChannelSet() == juce::AudioChannelSet::mono());
}

void MDALeslieAudioProcessor::processBlock(juce::AudioBuffer<float>& audio, juce::MidiBuffer& midi)
{
    juce::ScopedNoDenormals noDenormals;
    midi.clear();
    if (audio.getNumChannels() < 2) return;
    if (getTotalNumInputChannels() == 1) audio.copyFrom(1, 0, audio, 0, 0, audio.getNumSamples());
    std::array<float, 9> p{};
    for (std::size_t i = 0; i < p.size(); ++i) p[i] = values_[i]->load(std::memory_order_relaxed);
    p[0] *= 0.5f;
    for (std::size_t i = 1; i <= 5; ++i) p[i] *= 0.01f;
    p[7] = (p[7] + 20.0f) / 40.0f;
    p[8] /= 200.0f;
    dsp_.setParameters(p);
    dsp_.process(audio.getWritePointer(0), audio.getWritePointer(1), audio.getNumSamples());
}

void MDALeslieAudioProcessor::setCurrentProgram(int index)
{
    if (!juce::isPositiveAndBelow(index, getNumPrograms())) return;
    constexpr std::array<float, 9> defaults{0.5f, 0.5f, 0.5f, 0.7f, 0.6f, 0.7f, 0.48f, 0.5f, 0.6f};
    for (std::size_t i = 0; i < ids.size(); ++i) apvts.getParameter(ids[i])->setValueNotifyingHost(defaults[i]);
    constexpr std::array<float, 3> speeds{0.5f, 1.0f, 0.0f};
    apvts.getParameter(ids[0])->setValueNotifyingHost(speeds[static_cast<std::size_t>(index)]);
    program_.store(index, std::memory_order_relaxed);
}

const juce::String MDALeslieAudioProcessor::getProgramName(int index)
{
    constexpr std::array<const char*, 3> names{"Slow Rotation", "Fast Rotation", "Stopped"};
    return juce::isPositiveAndBelow(index, getNumPrograms()) ? juce::String(names[static_cast<std::size_t>(index)]) : juce::String{};
}

juce::AudioProcessorEditor* MDALeslieAudioProcessor::createEditor()
{
    return new mda::RevivalEditor(*this, apvts, "Leslie");
}
void MDALeslieAudioProcessor::getStateInformation(juce::MemoryBlock& data)
{
    mda::saveState(apvts, getCurrentProgram(), data);
}
void MDALeslieAudioProcessor::setStateInformation(const void* data, int size)
{
    mda::restoreState(apvts, data, size, program_, getNumPrograms());
}

juce::AudioProcessorValueTreeState::ParameterLayout MDALeslieAudioProcessor::layout()
{
    juce::AudioProcessorValueTreeState::ParameterLayout parameters;
    parameters.add(std::make_unique<juce::AudioParameterChoice>(juce::ParameterID{ids[0], 1}, ids[0], juce::StringArray{"Stop", "Slow", "Fast"}, 1));
    constexpr std::array<float, 5> defaults{50.0f, 50.0f, 70.0f, 60.0f, 70.0f};
    for (std::size_t i = 1; i <= 5; ++i)
        parameters.add(std::make_unique<juce::AudioParameterFloat>(juce::ParameterID{ids[i], 1}, ids[i],
            juce::NormalisableRange<float>(0.0f, 100.0f, 0.1f), defaults[i - 1],
            juce::AudioParameterFloatAttributes().withLabel("%")));
    parameters.add(std::make_unique<juce::AudioParameterFloat>(juce::ParameterID{ids[6], 1}, ids[6],
        juce::NormalisableRange<float>(), 0.48f,
        juce::AudioParameterFloatAttributes().withLabel("Hz").withStringFromValueFunction([](float v, int) {
            return juce::String(10 * static_cast<int>(std::pow(10.0f, 1.179f + v)));
        })));
    parameters.add(std::make_unique<juce::AudioParameterFloat>(juce::ParameterID{ids[7], 1}, ids[7],
        juce::NormalisableRange<float>(-20.0f, 20.0f, 0.1f), 0.0f, juce::AudioParameterFloatAttributes().withLabel("dB")));
    parameters.add(std::make_unique<juce::AudioParameterFloat>(juce::ParameterID{ids[8], 1}, ids[8],
        juce::NormalisableRange<float>(0.0f, 200.0f, 0.1f), 120.0f, juce::AudioParameterFloatAttributes().withLabel("%")));
    return parameters;
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter() { return new MDALeslieAudioProcessor(); }
