cmake_minimum_required(VERSION 3.22)
project(VocalPitch VERSION 0.1.0)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_OSX_DEPLOYMENT_TARGET "10.13" CACHE STRING "")

include(FetchContent)
FetchContent_Declare(
    JUCE
    GIT_REPOSITORY https://github.com/juce-framework/JUCE.git
    GIT_TAG 8.0.4
)
FetchContent_MakeAvailable(JUCE)

juce_add_plugin(VocalPitch
    COMPANY_NAME "Michael Bilenko"
    PLUGIN_MANUFACTURER_CODE Flor
    PLUGIN_CODE Vpch
    FORMATS AU
    PRODUCT_NAME "VocalPitch"
    BUNDLE_ID com.michbilenko.vocalpitch
    IS_SYNTH FALSE
    NEEDS_MIDI_INPUT FALSE
    NEEDS_MIDI_OUTPUT FALSE
    COPY_PLUGIN_AFTER_BUILD TRUE
)

target_sources(VocalPitch PRIVATE
    Source/PluginProcessor.cpp
    Source/PluginEditor.cpp
    Source/YinPitchDetector.cpp
    Source/PianoKeyboard.cpp
    Source/PitchGraph.cpp
)

target_include_directories(VocalPitch PRIVATE Source)

target_compile_definitions(VocalPitch PRIVATE
    JUCE_WEB_BROWSER=0
    JUCE_USE_CURL=0
    JUCE_VST3_CAN_REPLACE_VST2=0
)

target_link_libraries(VocalPitch PRIVATE
    juce::juce_audio_utils
    juce::juce_dsp
    juce::juce_audio_plugin_client
    juce::juce_core
    juce::juce_graphics
    juce::juce_gui_basics
    juce::juce_gui_extra
    juce::juce_recommended_config_flags
    juce::juce_recommended_lto_flags
    juce::juce_recommended_warning_flags
)
