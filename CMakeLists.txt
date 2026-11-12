cmake_minimum_required(VERSION 3.22)
project(SoloWah VERSION 1.0.0 LANGUAGES C CXX)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

option(SOLOWAH_BUILD_TOOLS "Build the GUI snapshot tool (renders the editor to PNG)" OFF)
option(SOLOWAH_MINIMAL_X11 "Linux only: skip optional X11 extensions (XRandR/Xinerama/Xcursor)" OFF)

# JUCE: use a local checkout if -DJUCE_PATH=... is given, otherwise download it.
if(JUCE_PATH)
    add_subdirectory(${JUCE_PATH} JUCE EXCLUDE_FROM_ALL)
else()
    include(FetchContent)
    FetchContent_Declare(JUCE
        GIT_REPOSITORY https://github.com/juce-framework/JUCE.git
        GIT_TAG 8.0.4
        GIT_SHALLOW ON)
    FetchContent_MakeAvailable(JUCE)
endif()

set(SOLOWAH_SOURCES
    Source/PluginProcessor.cpp
    Source/PluginEditor.cpp)

set(SOLOWAH_DEFS
    JUCE_WEB_BROWSER=0
    JUCE_USE_CURL=0
    JUCE_VST3_CAN_REPLACE_VST2=0
    JUCE_DISPLAY_SPLASH_SCREEN=0)
if(SOLOWAH_MINIMAL_X11)
    list(APPEND SOLOWAH_DEFS JUCE_USE_XRANDR=0 JUCE_USE_XINERAMA=0 JUCE_USE_XCURSOR=0)
endif()

set(SOLOWAH_FORMATS VST3)
if(APPLE)
    list(APPEND SOLOWAH_FORMATS AU)
endif()

juce_add_plugin(SoloWah
    COMPANY_NAME "SoloWah"
    PLUGIN_MANUFACTURER_CODE Slwh
    PLUGIN_CODE Swah
    FORMATS ${SOLOWAH_FORMATS}
    PRODUCT_NAME "Solo Wah"
    IS_SYNTH FALSE
    NEEDS_MIDI_INPUT TRUE
    NEEDS_MIDI_OUTPUT FALSE
    VST3_CATEGORIES Fx Filter
    COPY_PLUGIN_AFTER_BUILD TRUE)

target_sources(SoloWah PRIVATE ${SOLOWAH_SOURCES})
target_compile_definitions(SoloWah PUBLIC ${SOLOWAH_DEFS})
target_link_libraries(SoloWah
    PRIVATE juce::juce_audio_processors
    PUBLIC  juce::juce_recommended_config_flags
            juce::juce_recommended_warning_flags)

if(SOLOWAH_BUILD_TOOLS)
    juce_add_console_app(SoloWahSnapshot PRODUCT_NAME "SoloWahSnapshot")
    target_sources(SoloWahSnapshot PRIVATE tools/snapshot.cpp ${SOLOWAH_SOURCES})
    target_compile_definitions(SoloWahSnapshot PRIVATE ${SOLOWAH_DEFS} JUCE_STANDALONE_APPLICATION=1)
    target_link_libraries(SoloWahSnapshot
        PRIVATE juce::juce_audio_processors juce::juce_recommended_config_flags)
endif()
