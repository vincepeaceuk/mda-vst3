cmake_minimum_required(VERSION 3.22)
project(MDARevival VERSION 0.1.0 LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_OSX_DEPLOYMENT_TARGET "11.0" CACHE STRING "Oldest supported macOS")
set(JUCE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/deps/JUCE" CACHE PATH "JUCE 9.0.5 checkout")
add_subdirectory("${JUCE_PATH}" JUCE)
include(CTest)

function(add_mda_plugin model product code synth category)
    set(target "MDA_${model}")
    juce_add_plugin(${target}
        COMPANY_NAME "MDA Revival"
        BUNDLE_ID "uk.richardpeace.mda.${model}"
        PLUGIN_MANUFACTURER_CODE MdRv
        PLUGIN_CODE ${code}
        PRODUCT_NAME "${product}"
        DESCRIPTION "Classic MDA ${model}, revived for macOS"
        IS_SYNTH ${synth}
        NEEDS_MIDI_INPUT ${synth}
        NEEDS_MIDI_OUTPUT FALSE
        EDITOR_WANTS_KEYBOARD_FOCUS FALSE
        VST3_CATEGORIES ${category}
        FORMATS VST3
        COPY_PLUGIN_AFTER_BUILD FALSE)
    juce_generate_juce_header(${target})
    target_sources(${target} PRIVATE "plugins/${model}/PluginProcessor.cpp")
    target_include_directories(${target} PRIVATE shared)
    if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/shared/RevivalEditor.cpp")
        target_sources(${target} PRIVATE shared/RevivalEditor.cpp)
    endif()
    target_compile_definitions(${target} PUBLIC
        JUCE_WEB_BROWSER=0 JUCE_USE_CURL=0 JUCE_VST3_CAN_REPLACE_VST2=0
        JUCE_REPORT_APP_USAGE=0 JUCE_DISPLAY_SPLASH_SCREEN=0)
    target_link_libraries(${target} PRIVATE juce::juce_audio_utils
        PUBLIC juce::juce_recommended_config_flags)
endfunction()

add_mda_plugin(Piano "mda Piano" RvPi TRUE "Instrument;Sampler")
add_mda_plugin(EPiano "mda ePiano" RvEp TRUE "Instrument;Sampler")
add_mda_plugin(DX10 "mda DX10" RvDx TRUE "Instrument;Synth")
add_mda_plugin(JX10 "mda JX10" RvJx TRUE "Instrument;Synth")
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/plugins/Leslie/PluginProcessor.cpp")
    add_mda_plugin(Leslie "mda Leslie" RvLs FALSE "Fx;Modulation")
endif()

juce_add_console_app(MDAPluginTests PRODUCT_NAME "MDA VST3 tests")
juce_generate_juce_header(MDAPluginTests)
target_sources(MDAPluginTests PRIVATE tests/PluginTests.cpp)
target_compile_definitions(MDAPluginTests PRIVATE
    JUCE_PLUGINHOST_VST3=1 JUCE_PLUGINHOST_AU=0 JUCE_WEB_BROWSER=0 JUCE_USE_CURL=0)
target_link_libraries(MDAPluginTests PRIVATE juce::juce_audio_utils
    juce::juce_cryptography juce::juce_recommended_config_flags)

if(BUILD_TESTING)
    foreach(model Piano EPiano DX10 JX10 Leslie)
        if(TARGET MDA_${model})
            add_test(NAME ${model}_VST3 COMMAND MDAPluginTests
                "$<TARGET_BUNDLE_DIR:MDA_${model}_VST3>" --require-revival
                "${CMAKE_CURRENT_BINARY_DIR}/evidence")
        endif()
    endforeach()
endif()
