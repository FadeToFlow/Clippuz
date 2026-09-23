cmake_minimum_required(VERSION 3.22)
project(Clippuz VERSION 1.0.1)
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


juce_add_console_app(RegressionRender
    PRODUCT_NAME "RegressionRender")
 
juce_generate_juce_header(RegressionRender)
 
target_sources(RegressionRender PRIVATE
    Source/PluginProcessor.cpp   
    Source/EqFilter.cpp          
    Tools/regression_render.cpp) 
 
target_include_directories(RegressionRender PRIVATE Source)
 
target_compile_definitions(RegressionRender PRIVATE
    JUCE_WEB_BROWSER=0
    JUCE_USE_CURL=0
    JucePlugin_Name="Clippuz"
    JucePlugin_IsSynth=0
    JucePlugin_IsMidiEffect=0
    JucePlugin_WantsMidiInput=0
    JucePlugin_ProducesMidiOutput=0)
 
target_link_libraries(RegressionRender PRIVATE
    juce::juce_audio_utils
    juce::juce_dsp
    juce::juce_recommended_config_flags
    juce::juce_recommended_warning_flags)
 
enable_testing()
 
add_test(NAME regression_tests
    COMMAND python3 ${CMAKE_SOURCE_DIR}/Tools/run_regression_tests.py
            --render-exe $<TARGET_FILE:RegressionRender>
            --signals-dir ${CMAKE_SOURCE_DIR}/test_signals
            --reference-dir ${CMAKE_SOURCE_DIR}/test_signals/reference
            --eps 1e-4)
