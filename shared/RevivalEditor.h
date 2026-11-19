// SPDX-License-Identifier: AGPL-3.0-or-later
#pragma once
#include <JuceHeader.h>
#include <functional>
#include <memory>
#include <vector>

namespace mda {
class RevivalEditor final : public juce::AudioProcessorEditor, private juce::Timer {
public:
    RevivalEditor(juce::AudioProcessor&, juce::AudioProcessorValueTreeState&, const juce::String& model,
                  juce::MidiKeyboardState* = nullptr, std::function<void()> panic = {});
    ~RevivalEditor() override;
    void paint(juce::Graphics&) override;
    void resized() override;

private:
    struct Style;
    struct Control;
    struct Section { juce::String name; std::vector<int> parameters; };
    void timerCallback() override;
    void chooseProgram(int);
    std::unique_ptr<Style> style_;
    juce::AudioProcessor& processor_;
    juce::String model_, subtitle_;
    juce::Colour accent_;
    std::vector<Section> sections_;
    std::vector<std::unique_ptr<Control>> controls_;
    juce::ComboBox presets_;
    juce::TextButton previous_{"<"}, next_{">"}, panic_{"ALL OFF"};
    std::unique_ptr<juce::MidiKeyboardComponent> keyboard_;
    juce::TooltipWindow tooltips_{this, 650};
    int rows_ = 4;
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(RevivalEditor)
};
}
