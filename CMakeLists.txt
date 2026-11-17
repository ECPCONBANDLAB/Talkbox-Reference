cmake_minimum_required(VERSION 3.20)
project(TalkBoxReference VERSION 1.3.0 LANGUAGES CXX)

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
# Set -DTB_BUILD_JUCE=ON and -DJUCE_PATH=/path/to/JUCE to build VST3/AU/Standalone.
# Default OFF so headless CI (DSP-only) builds without JUCE download.
# JUCE 7.0.12 has no CLAP wrapper; see the formats note inside the block below.
# ---------------------------------------------------------------------------
option(TB_BUILD_JUCE "Build JUCE plugin (VST3/AU/Standalone)" OFF)

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

  # -------------------------------------------------------------------------
  # Formats. JUCE 7.0.12 has wrappers for VST3, AU, LV2, Standalone, Unity,
  # AAX and ARA — it has no CLAP wrapper (no juce_audio_plugin_client/CLAP
  # directory, no CLAP entry in _juce_get_platform_plugin_kinds). Requesting
  # CLAP here is silently ignored, which is why this comment exists: CLAP
  # output would need a third-party wrapper, not a flag.
  # AU is macOS-only and is dropped automatically on other platforms.
  # -------------------------------------------------------------------------
  set(TB_PLUGIN_FORMATS VST3 AU Standalone)

  juce_add_plugin(TalkBoxReference
    VERSION ${PROJECT_VERSION}
    DESCRIPTION "Physically accurate hardware talkbox emulation - mic-playable, ultra-low CPU"
    COMPANY_NAME "TalkBox Reference"
    PLUGIN_MANUFACTURER_CODE TbxR
    PLUGIN_CODE Tbx1
    PLUGIN_NAME "TalkBox Reference"
    FORMATS ${TB_PLUGIN_FORMATS}
    VST3_CATEGORIES Fx
    NEEDS_MIDI_INPUT TRUE
    NEEDS_MIDI_OUTPUT FALSE
    IS_SYNTH TRUE
    # Sidechain for mic control: bus 1 (mono input) is the talkbox mic.
    # Standalone feeds the mic through its own audio input instead.
    )

  juce_generate_juce_header(${PROJECT_NAME})

  # Glob the UI sources rather than listing them: VowelPad.cpp was added as a
  # new translation unit and a hand-maintained list silently omitted it, which
  # only surfaced as an MSVC link error at packaging time.
  file(GLOB TB_UI_SOURCES CONFIGURE_DEPENDS
    ${CMAKE_CURRENT_SOURCE_DIR}/src/ui/*.cpp)
  target_sources(${PROJECT_NAME} PRIVATE
    src/plugin/PluginProcessor.cpp
    src/plugin/PluginEditor.cpp
    ${TB_UI_SOURCES}
  )
  target_include_directories(${PROJECT_NAME} PRIVATE src/dsp src/plugin src/ui)
  target_compile_definitions(${PROJECT_NAME} PUBLIC
    JUCE_WEB_BROWSER=0 JUCE_USE_CURL=0 JUCE_VST3_CAN_REPLACE_VST2=0
    # Suppress the "Made with JUCE" splash in the Standalone build. JUCE 7 only
    # enforces the badge under the free Personal tier; this project distributes
    # under the GPLv3 alternative (see LICENSE-NOTES.md), which does not require
    # it. If the licence position is ever revisited, remove this line.
    JUCE_DISPLAY_SPLASH_SCREEN=0)

  # Factory presets embedded as BinaryData so the plugin is self-contained.
  # NOTE: juce_add_binary_data() creates its OWN target (<name>) and the header
  # it generates must be #included by hand — it is NOT attached to the plugin
  # target automatically. Do not pass ${PROJECT_NAME} as the target name here:
  # juce_add_plugin() already called add_library(PROJECT_NAME STATIC), and a
  # second add_library() with the same name is a hard CMake error.
  # The preset browser UI is still a placeholder, so nothing reads this data yet.
  juce_add_binary_data(TalkBoxReferenceBinaryData SOURCES presets/factory_presets.json)
  target_link_libraries(${PROJECT_NAME} PRIVATE TalkBoxReferenceBinaryData)

  target_link_libraries(${PROJECT_NAME} PRIVATE
    juce::juce_audio_utils
    juce::juce_dsp)
endif()

# ---------------------------------------------------------------------------
# Install (Linux .tar.gz layout; Windows/macOS via packaging/ scripts)
# ---------------------------------------------------------------------------
include(GNUInstallDirs)
install(FILES README.md LICENSE CHANGELOG.md DESTINATION ${CMAKE_INSTALL_DATADIR}/talkbox-reference)
install(DIRECTORY presets/ DESTINATION ${CMAKE_INSTALL_DATADIR}/talkbox-reference/presets)
install(DIRECTORY docs/ DESTINATION ${CMAKE_INSTALL_DATADIR}/talkbox-reference/docs)
