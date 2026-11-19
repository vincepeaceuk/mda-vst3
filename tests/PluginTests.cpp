#include <JuceHeader.h>
#include <cmath>
#include <cstdlib>
#include <iostream>
#include <memory>

namespace {
int failures = 0;
void check(bool pass, const juce::String& message)
{
    std::cout << (pass ? "PASS " : "FAIL ") << message << std::endl;
    if (!pass) ++failures;
}

std::unique_ptr<juce::AudioPluginInstance> load(juce::VST3PluginFormat& format,
                                              const juce::PluginDescription& desc,
                                              double rate, int block)
{
    std::srand(1);
    juce::String error;
    auto instance = format.createInstanceFromDescription(desc, rate, block, error);
    if (!instance) std::cerr << error << std::endl;
    return instance;
}

void runAudio(juce::AudioPluginInstance& plugin, bool instrument, double rate, int block,
              juce::Array<juce::var>& evidence)
{
    plugin.setRateAndBufferSizeDetails(rate, block);
    plugin.prepareToPlay(rate, block);
    const auto channels = juce::jmax(2, plugin.getTotalNumOutputChannels());
    juce::AudioBuffer<float> audio(channels, block);
    juce::MidiBuffer midi;
    bool finite = true;
    double energy = 0.0;
    float peak = 0.0f;
    const int blocks = static_cast<int>(rate * 0.12) / block + 1;
    for (int n = 0; n < blocks; ++n) {
        audio.clear();
        midi.clear();
        if (instrument) {
            if (n == 0) midi.addEvent(juce::MidiMessage::noteOn(1, 60, (juce::uint8) 100), 0);
            if (n == blocks - 2) midi.addEvent(juce::MidiMessage::noteOff(1, 60), block / 2);
        } else {
            for (int i = 0; i < block; ++i) {
                const float v = 0.1f * std::sin(juce::MathConstants<double>::twoPi * 440.0 * (n * block + i) / rate);
                for (int ch = 0; ch < plugin.getTotalNumInputChannels(); ++ch) audio.setSample(ch, i, v);
            }
        }
        plugin.processBlock(audio, midi);
        for (int ch = 0; ch < 2; ++ch) {
            const auto* data = audio.getReadPointer(ch);
            for (int i = 0; i < block; ++i) {
                finite &= std::isfinite(data[i]);
                energy += static_cast<double>(data[i]) * data[i];
                peak = juce::jmax(peak, std::abs(data[i]));
            }
        }
    }
    check(finite && peak < 100.0f, "finite bounded audio at " + juce::String(rate) + "/" + juce::String(block));
    check(energy > 1.0e-10, "audible output at " + juce::String(rate));
    auto* row = new juce::DynamicObject();
    row->setProperty("rate", rate);
    row->setProperty("block", block);
    row->setProperty("energy", energy);
    row->setProperty("peak", peak);
    evidence.add(juce::var(row));
    plugin.releaseResources();
}
}

int main(int argc, char** argv)
{
    juce::ScopedJuceInitialiser_GUI initialise;
    if (argc < 2) { std::cerr << "Usage: MDAPluginTests plugin.vst3 [--require-revival] [evidence-dir]\n"; return 2; }
    const bool requireRevival = argc > 2 && juce::String(argv[2]) == "--require-revival";
    const auto output = argc > 3 ? juce::File::getCurrentWorkingDirectory().getChildFile(argv[3])
                                : juce::File::getCurrentWorkingDirectory().getChildFile("evidence");
    output.createDirectory();
    juce::VST3PluginFormat format;
    juce::OwnedArray<juce::PluginDescription> types;
    format.findAllTypesForFile(types, juce::File(argv[1]).getFullPathName());
    check(types.size() == 1, "VST3 scan finds one plugin");
    if (types.isEmpty()) return 1;
    const auto& description = *types[0];
    std::cout << "Testing " << description.name << "\n";
    auto plugin = load(format, description, 44100.0, 256);
    check(plugin != nullptr, "VST3 instantiates");
    if (!plugin) return 1;
    check(plugin->getTotalNumOutputChannels() == 2, "stereo output");
    const bool instrument = description.isInstrument;
    check(plugin->acceptsMidi() == instrument, "MIDI capability matches plugin type");
    check(!plugin->getParameters().isEmpty(), "automatable parameters");

    // Host recall must retain edited values and the selected factory program.
    const int selected = juce::jmin(2, plugin->getNumPrograms() - 1);
    plugin->setCurrentProgram(selected);
    auto* first = plugin->getParameters().getFirst();
    first->setValueNotifyingHost(0.31f);
    const float savedValue = first->getValue();
    juce::MemoryBlock state;
    plugin->getStateInformation(state);
    plugin->setCurrentProgram(0);
    first->setValueNotifyingHost(0.83f);
    plugin->setStateInformation(state.getData(), static_cast<int>(state.getSize()));
    check(std::abs(first->getValue() - savedValue) < 1.0e-5f, "edited parameter survives state restore");
    if (requireRevival) check(plugin->getCurrentProgram() == selected, "factory program survives state restore");
    const char bad[] = "not a plugin state";
    plugin->setStateInformation(bad, sizeof(bad));
    check(std::abs(first->getValue() - savedValue) < 1.0e-5f, "malformed state leaves parameters intact");

    juce::Array<juce::var> evidence;
    for (double rate : {44100.0, 48000.0, 96000.0, 192000.0}) {
        for (int block : {32, 256, 1024}) {
            plugin->setCurrentProgram(0);
            runAudio(*plugin, instrument, rate, block, evidence);
        }
    }
    // Every factory sound is reachable and safe to render.
    for (int program = 0; program < plugin->getNumPrograms(); ++program) {
        check(plugin->getProgramName(program).isNotEmpty(), "named factory program " + juce::String(program));
        plugin->setCurrentProgram(program);
        runAudio(*plugin, instrument, 48000.0, 256, evidence);
    }

    // Exercise parameter endpoints together at the highest tested sample rate.
    if (!instrument) {
        for (auto* parameter : plugin->getParameters()) parameter->setValueNotifyingHost(1.0f);
        runAudio(*plugin, false, 192000.0, 32, evidence);
    }
    plugin->setCurrentProgram(0);
    {
        std::unique_ptr<juce::AudioProcessorEditor> editor(plugin->createEditorIfNeeded());
        check(editor != nullptr, "editor opens");
        if (editor) {
            if (requireRevival) {
                check(editor->getWidth() >= 600 && editor->getWidth() <= 800
                      && editor->getHeight() <= 560, "compact vintage panel dimensions");
            }
            editor->setVisible(true);
            auto picture = editor->createComponentSnapshot(editor->getLocalBounds(), true, 2.0f);
            auto stream = output.getChildFile(description.name + ".png").createOutputStream();
            if (stream) { stream->setPosition(0); stream->truncate(); juce::PNGImageFormat().writeImageToStream(picture, *stream); }
            plugin->editorBeingDeleted(editor.get());
        }
    }
    output.getChildFile(description.name + "-audio.json").replaceWithText(juce::JSON::toString(juce::var(evidence)));
    std::cout << "RESULT " << failures << " failures\n";
    return failures == 0 ? 0 : 1;
}
