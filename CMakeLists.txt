cmake_minimum_required(VERSION 3.22)

# macOS 11 (Big Sur) on Intel is the minimum; build universal (x86_64 + arm64) on CI.
# No -march flags anywhere: the code is scalar C++ and runs on any x86-64 (no AVX2 needed).
set(CMAKE_OSX_DEPLOYMENT_TARGET "11.0" CACHE STRING "Minimum macOS version")
if(APPLE AND NOT CMAKE_OSX_ARCHITECTURES)
    set(CMAKE_OSX_ARCHITECTURES "x86_64;arm64" CACHE STRING "macOS architectures")
endif()
if(MSVC)
    set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")  # static CRT: no VC++ redist needed
endif()
project(Mouthpiece VERSION 0.1.0 LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# ---- Core engine (no JUCE dependency, unit-testable) ----
add_library(mouthpiece_engine STATIC
    Source/engine/Engine.cpp
    Source/engine/Carrier.cpp
    Source/engine/Vocoders.cpp
    Source/engine/Presets.cpp)
target_include_directories(mouthpiece_engine PUBLIC Source/engine)
set_target_properties(mouthpiece_engine PROPERTIES POSITION_INDEPENDENT_CODE ON)

option(MOUTHPIECE_BUILD_TESTS "Build engine tests" ON)
if(MOUTHPIECE_BUILD_TESTS)
    add_executable(mouthpiece_tests tests/EngineTests.cpp)
    target_link_libraries(mouthpiece_tests PRIVATE mouthpiece_engine)
    enable_testing()
    add_test(NAME engine COMMAND mouthpiece_tests)
    # demo reel renderer (engine only): mouthpiece_render_demo voice.wav out.wav [preset names...]
    add_executable(mouthpiece_render_demo tools/RenderDemo.cpp)
    target_link_libraries(mouthpiece_render_demo PRIVATE mouthpiece_engine)
endif()

option(MOUTHPIECE_BUILD_PLUGIN "Build the JUCE plugin" ON)
if(NOT MOUTHPIECE_BUILD_PLUGIN)
    return()
endif()

# ---- JUCE ----
set(JUCE_DIR "" CACHE PATH "Path to a local JUCE checkout (optional)")
if(JUCE_DIR)
    add_subdirectory(${JUCE_DIR} JUCE)
else()
    include(FetchContent)
    FetchContent_Declare(JUCE
        GIT_REPOSITORY https://github.com/juce-framework/JUCE.git
        GIT_TAG 8.0.4
        GIT_SHALLOW TRUE)
    FetchContent_MakeAvailable(JUCE)
endif()

# An effect (the voice comes in on the main input) that also takes MIDI to play the carrier synth,
# with an optional sidechain bus for an external carrier or modulator.
juce_add_plugin(Mouthpiece
    COMPANY_NAME "MouthpieceAudio"
    PLUGIN_MANUFACTURER_CODE Mthp
    PLUGIN_CODE Mpv1
    FORMATS VST3 Standalone
    PRODUCT_NAME "Mouthpiece"
    IS_SYNTH FALSE
    NEEDS_MIDI_INPUT TRUE
    NEEDS_MIDI_OUTPUT FALSE
    IS_MIDI_EFFECT FALSE
    EDITOR_WANTS_KEYBOARD_FOCUS TRUE
    COPY_PLUGIN_AFTER_BUILD FALSE
    VST3_CATEGORIES Fx Vocals)

target_sources(Mouthpiece PRIVATE
    Source/plugin/PluginProcessor.cpp
    Source/plugin/PluginEditor.cpp)

target_compile_definitions(Mouthpiece PUBLIC
    JUCE_WEB_BROWSER=0
    JUCE_USE_CURL=0
    JUCE_VST3_CAN_REPLACE_VST2=0
    JUCE_DISPLAY_SPLASH_SCREEN=0)

target_link_libraries(Mouthpiece
    PRIVATE
        mouthpiece_engine
        juce::juce_audio_utils
    PUBLIC
        juce::juce_recommended_config_flags
        juce::juce_recommended_lto_flags
        juce::juce_recommended_warning_flags)

# ---- Optional: render the editor in every theme to PNG (docs screenshots) ----
option(MOUTHPIECE_BUILD_SCREENSHOT "Build the screenshot tool" OFF)
if(MOUTHPIECE_BUILD_SCREENSHOT)
    juce_add_console_app(mouthpiece_screenshot PRODUCT_NAME "mouthpiece_screenshot")
    target_sources(mouthpiece_screenshot PRIVATE
        tools/Screenshot.cpp
        Source/plugin/PluginProcessor.cpp
        Source/plugin/PluginEditor.cpp)
    target_compile_definitions(mouthpiece_screenshot PRIVATE
        JucePlugin_Name="Mouthpiece"
        JUCE_MODAL_LOOPS_PERMITTED=1
        JUCE_WEB_BROWSER=0
        JUCE_USE_CURL=0)
    target_link_libraries(mouthpiece_screenshot PRIVATE
        mouthpiece_engine
        juce::juce_audio_utils
        juce::juce_recommended_config_flags)
endif()
