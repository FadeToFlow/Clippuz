cmake_minimum_required(VERSION 3.22)
project(Clippuz VERSION 1.0.0)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

include(FetchContent)
FetchContent_Declare(JUCE
    GIT_REPOSITORY https://github.com/juce-framework/JUCE.git
    GIT_TAG 8.0.4
    GIT_SHALLOW TRUE
    GIT_CONFIG    advice.detachedHead=false)
FetchContent_MakeAvailable(JUCE)

juce_add_plugin(Clippuz
    COMPANY_NAME            FadeToFlow
    BUNDLE_ID               com.fadetoflow.clippuz
    PLUGIN_MANUFACTURER_CODE FTFl
    PLUGIN_CODE             Clpz
    FORMATS                 VST3 LV2 AU Standalone
    LV2URI                  "https://github.com/FadeToFlow/Clippuz"
    VST3_CATEGORIES         Fx Distortion
    PRODUCT_NAME            "Clippuz")

juce_generate_juce_header(Clippuz)

target_sources(Clippuz PRIVATE
    Source/PluginProcessor.cpp
    Source/PluginEditor.cpp
    Source/EqFilter.cpp)

target_compile_definitions(Clippuz PUBLIC
    JUCE_WEB_BROWSER=0 JUCE_USE_CURL=0 JUCE_VST3_CAN_REPLACE_VST2=0)

target_link_libraries(Clippuz PRIVATE
    juce::juce_audio_utils juce::juce_dsp
    PUBLIC juce::juce_recommended_config_flags juce::juce_recommended_warning_flags)