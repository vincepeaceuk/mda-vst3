#include "PluginProcessor.h"
#include <cmath>
#include <iostream>
#include <memory>

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter();

int main(int argc, char** argv)
{
    juce::ScopedJuceInitialiser_GUI gui;
    int failures = 0;
    const auto check = [&failures](bool result, const char* text) {
        std::cout << (result ? "PASS " : "FAIL ") << text << std::endl;
        if (!result) ++failures;
    };
    std::unique_ptr<juce::AudioProcessor> plugin(createPluginFilter());
    const int selected = juce::jmin(2, plugin->getNumPrograms() - 1);
    plugin->setCurrentProgram(selected);
    auto* parameter = plugin->getParameters().getFirst();
    parameter->setValueNotifyingHost(0.31f);
    const auto value = parameter->getValue();
    juce::MemoryBlock state;
    plugin->getStateInformation(state);
    plugin->setCurrentProgram(0);
    parameter->setValueNotifyingHost(0.8f);
    plugin->setStateInformation(state.getData(), static_cast<int>(state.getSize()));
    check(plugin->getCurrentProgram() == selected, "selected factory program is saved");
    check(std::abs(parameter->getValue() - value) < 1.0e-5f, "edited settings are saved");
    if (argc > 1 && juce::String(argv[1]) == "--red") return failures == 0 ? 0 : 1;
    plugin->setCurrentProgram(-1);
    plugin->setCurrentProgram(plugin->getNumPrograms());
    check(plugin->getCurrentProgram() == selected, "invalid program indices are ignored");
    check(plugin->getProgramName(-1).isEmpty(), "invalid program name is safe");

    std::unique_ptr<juce::AudioProcessorEditor> editor(plugin->createEditorAndMakeActive());
    check(editor != nullptr, "editor is available");
    if (editor) {
        check(editor->getName() == "MDA Revival Editor", "custom vintage editor is used");
        check(editor->getWidth() >= 600 && editor->getWidth() <= 800 && editor->getHeight() <= 560,
              "compact panel dimensions");
        if (argc > 1) {
            auto directory = juce::File::getCurrentWorkingDirectory().getChildFile(argv[1]);
            directory.createDirectory();
            auto picture = editor->createComponentSnapshot(editor->getLocalBounds(), true, 2.0f);
            auto stream = directory.getChildFile(juce::String(MDA_TEST_MODEL) + ".png").createOutputStream();
            if (stream) { stream->setPosition(0); stream->truncate(); juce::PNGImageFormat().writeImageToStream(picture, *stream); }
        }
    }
    std::cout << "RESULT " << failures << " failures\n";
    return failures == 0 ? 0 : 1;
}
