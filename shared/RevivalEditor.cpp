// SPDX-License-Identifier: AGPL-3.0-or-later
#include "RevivalEditor.h"

namespace mda {
namespace {
const juce::Colour panel{0xffedaa75}, ink{0xff3e2624}, muted{0xff6a3828}, paper{0xfffff0d5};
const juce::Colour sunset{0xffb44620}, headerStart{0xff622b24}, headerEnd{0xff91402b}, horizon{0xffffa158}, glow{0xffffd3a1};
juce::Font mono(float size, bool bold = false)
{
    return juce::Font(juce::FontOptions(juce::Font::getDefaultMonospacedFontName(), size,
                                      bold ? juce::Font::bold : juce::Font::plain));
}
juce::String shortName(juce::String name)
{
    return name.replace("Envelope ", "").replace("Velocity to ", "Vel > ")
        .replace("Velocity Sensitivity", "Velocity").replace("Hardness Offset", "Hardness")
        .replace("Muffling Filter", "Muffle").replace("Muffling", "Muffle")
        .replace("Stereo Width", "Stereo").replace("Random Detuning", "Random Tune")
        .replace("Random Tuning", "Random Tune").replace("Stretch Tuning", "Stretch")
        .replace("Fine Tuning", "Fine Tune").replace("Treble Boost", "Treble")
        .replace("FineTune", "Fine Tune").toUpperCase();
}
}

struct RevivalEditor::Style final : juce::LookAndFeel_V4 {
    explicit Style(juce::Colour colour) : accent(colour)
    {
        setColour(juce::ComboBox::backgroundColourId, paper);
        setColour(juce::ComboBox::textColourId, ink);
        setColour(juce::ComboBox::outlineColourId, muted);
        setColour(juce::ComboBox::arrowColourId, ink);
        setColour(juce::PopupMenu::backgroundColourId, paper);
        setColour(juce::PopupMenu::textColourId, ink);
        setColour(juce::PopupMenu::highlightedBackgroundColourId, ink);
        setColour(juce::PopupMenu::highlightedTextColourId, paper);
        setColour(juce::TextButton::buttonColourId, panel.brighter(0.08f));
        setColour(juce::TextButton::textColourOffId, ink);
        setColour(juce::TooltipWindow::backgroundColourId, paper);
        setColour(juce::TooltipWindow::textColourId, ink);
    }
    juce::Font getComboBoxFont(juce::ComboBox&) override { return mono(12.0f); }
    juce::Font getTextButtonFont(juce::TextButton&, int) override { return mono(11.0f, true); }
    juce::Font getPopupMenuFont() override { return mono(12.0f); }
    void drawButtonBackground(juce::Graphics& g, juce::Button& b, const juce::Colour&, bool over, bool down) override
    {
        const auto area = b.getLocalBounds().toFloat().reduced(0.5f);
        g.setColour(down ? muted : (over ? paper : panel.brighter(0.05f)));
        g.fillRect(area);
        g.setColour(muted);
        g.drawRect(area, 1.0f);
        if (!down) { g.setColour(juce::Colours::white.withAlpha(0.45f)); g.drawHorizontalLine(1, 1.0f, area.getRight() - 1.0f); }
    }
    void drawLinearSlider(juce::Graphics& g, int x, int y, int width, int height,
                          float position, float, float, juce::Slider::SliderStyle, juce::Slider& slider) override
    {
        const float mid = static_cast<float>(y) + static_cast<float>(height) * 0.5f;
        g.setColour(ink.withAlpha(0.22f));
        g.fillRect(static_cast<float>(x), mid - 2.0f, static_cast<float>(width), 4.0f);
        g.setColour(accent.withAlpha(slider.isMouseOverOrDragging() ? 1.0f : 0.7f));
        g.fillRect(static_cast<float>(x), mid - 1.0f, juce::jmax(0.0f, position - static_cast<float>(x)), 2.0f);
        g.setColour(ink);
        g.fillRect(position - 5.0f, mid - 7.0f, 10.0f, 14.0f);
        g.setColour(paper);
        g.fillRect(position - 4.0f, mid - 6.0f, 8.0f, 11.0f);
        g.setColour(accent);
        g.fillRect(position - 1.0f, mid - 4.0f, 2.0f, 7.0f);
        if (slider.hasKeyboardFocus(false)) { g.setColour(accent); g.drawRect(slider.getLocalBounds().toFloat().reduced(1.0f)); }
    }
    juce::Colour accent;
};

struct RevivalEditor::Control final : juce::Component {
    Control(juce::AudioProcessorValueTreeState& state, juce::RangedAudioParameter& p) : parameter(p)
    {
        label.setText(shortName(p.getName(100)), juce::dontSendNotification);
        label.setFont(mono(10.0f, true));
        label.setColour(juce::Label::textColourId, ink);
        label.setBorderSize({});
        value.setFont(mono(10.5f));
        value.setJustificationType(juce::Justification::centredRight);
        value.setColour(juce::Label::textColourId, muted);
        value.setBorderSize({});
        addAndMakeVisible(label); addAndMakeVisible(value);
        if (auto* choice = dynamic_cast<juce::AudioParameterChoice*>(&p)) {
            combo = std::make_unique<juce::ComboBox>();
            combo->addItemList(choice->choices, 1);
            combo->setTooltip(p.getName(100));
            addAndMakeVisible(*combo);
            comboAttachment = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment>(state, p.paramID, *combo);
        } else {
            slider.setSliderStyle(juce::Slider::LinearHorizontal);
            slider.setTextBoxStyle(juce::Slider::NoTextBox, false, 0, 0);
            slider.setWantsKeyboardFocus(true);
            slider.setTitle(p.getName(100));
            slider.setDescription("Drag to adjust. Double-click to reset. Arrow keys make fine adjustments.");
            slider.setTooltip(p.getName(100) + " | Double-click: reset");
            slider.setScrollWheelEnabled(false);
            addAndMakeVisible(slider);
            slider.onValueChange = [this] { refreshValue(); };
            sliderAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(state, p.paramID, slider);
            refreshValue();
        }
    }
    void refreshValue()
    {
        auto text = parameter.getText(parameter.convertTo0to1(static_cast<float>(slider.getValue())), 32).trim();
        const auto suffix = parameter.getLabel();
        if (suffix.isNotEmpty() && !text.endsWithIgnoreCase(suffix)) text += " " + suffix;
        value.setText(text, juce::dontSendNotification);
    }
    void resized() override
    {
        auto bounds = getLocalBounds();
        auto top = bounds.removeFromTop(15);
        label.setBounds(top.removeFromLeft(static_cast<int>(top.getWidth() * 0.55f)));
        value.setBounds(top);
        bounds.removeFromTop(1);
        if (combo) combo->setBounds(bounds.reduced(3, 1));
        else slider.setBounds(bounds);
    }
    juce::RangedAudioParameter& parameter;
    juce::Label label, value;
    juce::Slider slider;
    std::unique_ptr<juce::ComboBox> combo;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> sliderAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment> comboAttachment;
};

RevivalEditor::RevivalEditor(juce::AudioProcessor& processor, juce::AudioProcessorValueTreeState& state,
                             const juce::String& model, juce::MidiKeyboardState* keyboard,
                             std::function<void()> panic)
    : AudioProcessorEditor(processor), processor_(processor), model_(model)
{
    accent_ = sunset;
    if (model == "Piano") {
        subtitle_ = "ACOUSTIC PIANO";
        sections_ = {{"ENVELOPE / TONE", {0,1,2,3}}, {"TOUCH / RESPONSE", {4,5,6,8}}, {"SPACE / TUNING", {7,9,10,11}}};
    } else if (model == "EPiano") {
        subtitle_ = "ELECTRIC PIANO"; model_ = "ePiano";
        sections_ = {{"ENVELOPE / TONE", {0,1,2,3}}, {"MODULATION / DRIVE", {4,5,11,7}}, {"TOUCH / TUNING", {6,8,9,10}}};
    } else if (model == "DX10") {
        subtitle_ = "FM SYNTHESIZER";
        sections_ = {{"CARRIER", {0,1,2,13,11,12}}, {"MODULATOR", {3,4,5,6,7,8}}, {"VELOCITY / MOTION", {9,10,14,15}}};
    } else if (model == "JX10") {
        subtitle_ = "ANALOGUE SYNTHESIZER";
        sections_ = {{"OSCILLATORS / GLIDE", {0,1,2,3,4,5,22,23}}, {"FILTER / ENVELOPE", {6,7,8,10,11,12,13,14}}, {"AMPLITUDE / MODULATION", {15,16,17,18,19,20,9,21}}};
    } else {
        subtitle_ = "ROTARY SPEAKER";
        sections_ = {{"ROTATION", {0,8,6}}, {"DRUM / OUTPUT", {1,2,7}}, {"HORN", {3,4,5}}};
    }
    style_ = std::make_unique<Style>(accent_);
    setLookAndFeel(style_.get());
    setName("MDA Revival Editor");
    setOpaque(true);
    for (const auto& section : sections_) {
        rows_ = juce::jmax(rows_, static_cast<int>(section.parameters.size()));
        for (const int index : section.parameters) {
            if (auto* parameter = dynamic_cast<juce::RangedAudioParameter*>(processor.getParameters()[index])) {
                auto control = std::make_unique<Control>(state, *parameter);
                addAndMakeVisible(*control);
                controls_.push_back(std::move(control));
            }
        }
    }
    for (int i = 0; i < processor.getNumPrograms(); ++i)
        presets_.addItem(juce::String(i + 1).paddedLeft('0', 2) + "  " + processor.getProgramName(i), i + 1);
    presets_.setSelectedId(processor.getCurrentProgram() + 1, juce::dontSendNotification);
    presets_.setTooltip("Factory programs");
    presets_.onChange = [this] { chooseProgram(presets_.getSelectedId() - 1); };
    previous_.setTooltip("Previous factory program"); next_.setTooltip("Next factory program");
    previous_.onClick = [this] { chooseProgram((processor_.getCurrentProgram() + processor_.getNumPrograms() - 1) % processor_.getNumPrograms()); };
    next_.onClick = [this] { chooseProgram((processor_.getCurrentProgram() + 1) % processor_.getNumPrograms()); };
    addAndMakeVisible(presets_); addAndMakeVisible(previous_); addAndMakeVisible(next_);
    if (keyboard != nullptr) {
        keyboard_ = std::make_unique<juce::MidiKeyboardComponent>(*keyboard, juce::MidiKeyboardComponent::horizontalKeyboard);
        keyboard_->setAvailableRange(48, 84);
        keyboard_->setLowestVisibleKey(48);
        keyboard_->setScrollButtonsVisible(false);
        keyboard_->setKeyWidth(28.0f);
        keyboard_->setColour(juce::MidiKeyboardComponent::whiteNoteColourId, paper);
        keyboard_->setColour(juce::MidiKeyboardComponent::blackNoteColourId, ink);
        keyboard_->setColour(juce::MidiKeyboardComponent::keyDownOverlayColourId, accent_.withAlpha(0.7f));
        keyboard_->setColour(juce::MidiKeyboardComponent::keySeparatorLineColourId, muted.withAlpha(0.5f));
        addAndMakeVisible(*keyboard_);
        panic_.onClick = std::move(panic);
        panic_.setTooltip("Release all voices and sustain");
        addAndMakeVisible(panic_);
    }
    setSize(model == "JX10" ? 768 : 708, 174 + rows_ * 38 + (keyboard_ ? 62 : 0));
    startTimerHz(10);
}

RevivalEditor::~RevivalEditor()
{
    stopTimer();
    setLookAndFeel(nullptr);
}

void RevivalEditor::chooseProgram(int index)
{
    if (!juce::isPositiveAndBelow(index, processor_.getNumPrograms())) return;
    processor_.setCurrentProgram(index);
    processor_.updateHostDisplay(juce::AudioProcessor::ChangeDetails().withProgramChanged(true));
    presets_.setSelectedId(index + 1, juce::dontSendNotification);
}
void RevivalEditor::timerCallback()
{
    presets_.setSelectedId(processor_.getCurrentProgram() + 1, juce::dontSendNotification);
}

void RevivalEditor::paint(juce::Graphics& g)
{
    g.fillAll(panel);
    const auto bounds = getLocalBounds();
    g.setGradientFill(juce::ColourGradient(headerStart, 0.0f, 0.0f,
                                          headerEnd, static_cast<float>(getWidth()), 70.0f, false));
    g.fillRect(0, 0, getWidth(), 70);
    g.setColour(horizon); g.fillRect(0, 67, getWidth(), 3);
    g.setFont(juce::Font(juce::FontOptions(34.0f, juce::Font::bold)));
    g.setColour(paper); g.drawText("mda", 22, 10, 82, 45, juce::Justification::centredLeft);
    g.setFont(mono(27.0f)); g.drawText(model_, 117, 10, 230, 43, juce::Justification::centredLeft);
    g.setFont(mono(10.0f)); g.setColour(paper);
    g.drawText(subtitle_, 120, 49, 270, 13, juce::Justification::centredLeft);
    g.drawText("CLASSIC MDA", getWidth() - 158, 20, 135, 14, juce::Justification::centredRight);
    g.setColour(glow);
    g.drawText("REVIVAL / 01", getWidth() - 158, 37, 135, 14, juce::Justification::centredRight);
    g.setFont(mono(10.0f, true)); g.setColour(muted);
    g.drawText("PROGRAM", 22, 84, 80, 20, juce::Justification::centredLeft);
    const int columnWidth = (getWidth() - 44 - 24) / 3;
    for (int column = 0; column < 3; ++column) {
        const int x = 22 + column * (columnWidth + 12);
        const auto& section = sections_[static_cast<std::size_t>(column)];
        g.setColour(ink.withAlpha(0.035f)); g.fillRect(x - 6, 121, columnWidth + 12, rows_ * 38 + 30);
        g.setColour(accent_); g.fillRect(x, 124, 3, 10);
        g.setFont(mono(10.0f, true)); g.setColour(ink);
        g.drawText(section.name, x + 9, 120, columnWidth - 9, 18, juce::Justification::centredLeft);
        g.setColour(muted.withAlpha(0.35f)); g.drawHorizontalLine(140, static_cast<float>(x), static_cast<float>(x + columnWidth));
    }
    g.setColour(muted.withAlpha(0.4f)); g.drawHorizontalLine(getHeight() - 24, 22.0f, static_cast<float>(getWidth() - 22));
    g.setFont(mono(9.0f)); g.setColour(muted);
    g.drawText("PAUL KELLETT / MDA", 22, getHeight() - 21, 250, 16, juce::Justification::centredLeft);
    g.drawText("VST3  /  " JucePlugin_VersionString, getWidth() - 180, getHeight() - 21, 158, 16, juce::Justification::centredRight);
    for (const int x : {9, getWidth() - 10}) {
        for (const int y : {9, getHeight() - 10}) {
            g.setColour(muted.withAlpha(0.6f)); g.drawEllipse(static_cast<float>(x - 2), static_cast<float>(y - 2), 4.0f, 4.0f, 0.7f);
            g.drawLine(static_cast<float>(x - 1), static_cast<float>(y + 1), static_cast<float>(x + 1), static_cast<float>(y - 1), 0.7f);
        }
    }
}

void RevivalEditor::resized()
{
    previous_.setBounds(104, 81, 26, 26);
    presets_.setBounds(136, 81, getWidth() - (keyboard_ ? 276 : 200), 26);
    next_.setBounds(presets_.getRight() + 6, 81, 26, 26);
    if (keyboard_) {
        panic_.setBounds(getWidth() - 100, 81, 78, 26);
        keyboard_->setBounds(22, getHeight() - 83, getWidth() - 44, 49);
        keyboard_->setKeyWidth(static_cast<float>(getWidth() - 44) / 22.0f);
    }
    const int width = (getWidth() - 68) / 3;
    std::size_t index = 0;
    for (int column = 0; column < 3; ++column) {
        for (std::size_t row = 0; row < sections_[static_cast<std::size_t>(column)].parameters.size(); ++row) {
            controls_[index++]->setBounds(22 + column * (width + 12), 149 + static_cast<int>(row) * 38, width, 33);
        }
    }
}
}
