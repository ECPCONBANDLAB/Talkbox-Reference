cmake_minimum_required(VERSION 3.20)
project(TalkBoxReference VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# ---------------------------------------------------------------------------
# Headless DSP tests (no JUCE required) — always built
# ---------------------------------------------------------------------------
enable_testing()
add_executable(tb_tests tests/test_main.cpp)
target_include_directories(tb_tests PRIVATE src/dsp)

# Do NOT add -ffast-math here (nor -fassociative-math / -freciprocal-math).
# The DSP depends on IEEE-754 evaluation order: with reassociation enabled the
# LPC autocorrelation ordering changes enough that the whisper /i/ test tracks
# F2 (2186 Hz) as F1 instead of ~254 Hz → "LPC F1/F2 tracking" fails (39/40).
# -ffinite-math-only alone is harmless; the reassociation flags are not.
# The shipped plugin target carries none of these flags, so using them here
# would test a configuration that is never released.
if(MSVC)
  target_compile_options(tb_tests PRIVATE /O2 /W4)
else()
  target_compile_options(tb_tests PRIVATE -O2 -Wall -Wextra)
endif()

add_test(NAME tb_tests COMMAND tb_tests)

# ---------------------------------------------------------------------------
# JUCE plugin (optional: requires JUCE 7+ via FetchContent or local path)
# Set -DTB_BUILD_JUCE=ON and -DJUCE_PATH=/path/to/JUCE to build VST3/AU/CLAP.
# Default OFF so headless CI (DSP-only) builds without JUCE download.
# ---------------------------------------------------------------------------
option(TB_BUILD_JUCE "Build JUCE plugin (VST3/AU/CLAP/Standalone)" OFF)
option(TB_BUILD_STANDALONE "Also build Standalone app" ON)

if(TB_BUILD_JUCE)
  # JUCE must be provided via JUCE_PATH or fetched (offline-friendly: prefer local)
  if(DEFINED JUCE_PATH AND EXISTS "${JUCE_PATH}/CMakeLists.txt")
    add_subdirectory(${JUCE_PATH} juce_build)
  else()
    include(FetchContent)
    FetchContent_Declare(JUCE
      GIT_REPOSITORY https://github.com/juce-framework/JUCE.git
      GIT_TAG 7.0.12)
    FetchContent_MakeAvailable(JUCE)
  endif()

  juce_add_plugin(TalkBoxReference
    VERSION ${PROJECT_VERSION}
    DESCRIPTION "Physically accurate hardware talkbox emulation — mic-playable, ultra-low CPU"
    MANUFACTURER "TalkBox Reference"
    NAME "TalkBox Reference"
    PLUGIN_MANUFACTURER_CODE TbxR
    PLUGIN_CODE Tbx1
    FORMATS VST3 AU CLAP Standalone
    VST3_CATEGORIES "Fx Modulation"
    AU_MAIN_BUS_LAYOUTS "stereo"
    VST3_MAIN_BUS_LAYOUTS "stereo"
    CLAP_MAIN_BUS_LAYOUTS "stereo"
    STANDALONE_MAIN_BUS_LAYOUTS "stereo"
    NEEDS_MIDI_INPUT TRUE
    NEEDS_MIDI_OUTPUT FALSE
    IS_SYNTH TRUE
    IS_TRACKS_PLUGIN FALSE
    # Sidechain: VST3/AU/CLAP sidechain bus for mic control (see PluginProcessor)
    # JUCE 7 supports sidechain via AudioProcessor::getBusLayout + sidechain in processBlock.
    # Standalone uses direct mic input instead.
    )

  juce_generate_juce_header(${PROJECT_NAME})

  target_sources(${PROJECT_NAME} PRIVATE
    src/plugin/PluginProcessor.cpp
    src/plugin/PluginEditor.cpp
  )
  target_include_directories(${PROJECT_NAME} PRIVATE src/dsp src/plugin src/ui)
  target_compile_definitions(${PROJECT_NAME} PUBLIC
    JUCE_WEB_BROWSER=0 JUCE_USE_CURL=0 JUCE_VST3_CAN_REPLACE_VST2=0)
  juce_add_binary_data(${PROJECT_NAME} SOURCES presets/factory_presets.json)
  juce_add_module(${PROJECT_NAME} juce_audio_utils)
  target_link_libraries(${PROJECT_NAME} PRIVATE
    juce::juce_audio_utils
    juce::juce_dsp)
  juce_add_plugin(${PROJECT_NAME} TARGETS)
endif()

# ---------------------------------------------------------------------------
# Install (Linux .tar.gz layout; Windows/macOS via packaging/ scripts)
# ---------------------------------------------------------------------------
include(GNUInstallDirs)
install(FILES README.md LICENSE CHANGELOG.md DESTINATION ${CMAKE_INSTALL_DATADIR}/talkbox-reference)
install(DIRECTORY presets/ DESTINATION ${CMAKE_INSTALL_DATADIR}/talkbox-reference/presets)
install(DIRECTORY docs/ DESTINATION ${CMAKE_INSTALL_DATADIR}/talkbox-reference/docs)
