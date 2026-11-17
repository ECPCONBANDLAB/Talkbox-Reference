cmake_minimum_required(VERSION 3.22)
project(TalkBoxReference VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

option(TB_BUILD_TESTS "Build the validation suite" ON)
option(TB_BUILD_JUCE "Build the JUCE plugin targets (VST3/AU/CLAP/Standalone)" OFF)
option(TB_JUCE_PATH "Path to a local JUCE checkout (7.0.9+)" "")

if(MSVC)
  add_compile_options(/W4 /permissive-)
else()
  add_compile_options(-Wall -Wextra -Wpedantic)
endif()

# ---------------------------------------------------------------------------
# DSP core: header-only, no dependencies
# ---------------------------------------------------------------------------
add_library(talkbox_dsp INTERFACE)
target_include_directories(talkbox_dsp INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}/src/dsp)
target_compile_features(talkbox_dsp INTERFACE cxx_std_20)

# ---------------------------------------------------------------------------
# Framework-agnostic plugin core
# ---------------------------------------------------------------------------
add_library(talkbox_api STATIC src/plugin/TalkboxApi.cpp)
target_link_libraries(talkbox_api PUBLIC talkbox_dsp)

# ---------------------------------------------------------------------------
# Validation suite
# ---------------------------------------------------------------------------
if(TB_BUILD_TESTS)
  enable_testing()
  add_executable(run_tests tests/test_main.cpp)
  target_include_directories(run_tests PRIVATE src/dsp tests)
  target_link_libraries(run_tests PRIVATE talkbox_dsp)
  set_target_properties(run_tests PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/tests)
  add_test(NAME validation COMMAND run_tests)
  # The suite reads presets/factory_presets.json relative to the repo root.
  set_tests_properties(validation PROPERTIES WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR})
endif()

# ---------------------------------------------------------------------------
# Optional JUCE shell (VST3 / AU / CLAP / Standalone)
# ---------------------------------------------------------------------------
if(TB_BUILD_JUCE)
  if(TB_JUCE_PATH)
    add_subdirectory(${TB_JUCE_PATH} ${CMAKE_BINARY_DIR}/JUCE EXCLUDE_FROM_ALL)
  else()
    include(FetchContent)
    FetchContent_Declare(JUCE
      GIT_REPOSITORY https://github.com/juce-framework/JUCE.git
      GIT_TAG 7.0.12
      GIT_SHALLOW TRUE)
    FetchContent_MakeAvailable(JUCE)
  endif()

  juce_add_plugin(TalkBoxReference
    COMPANY_NAME "ECP Conband Lab"
    PLUGIN_MANUFACTURER_CODE Ecpc
    PLUGIN_CODE TbR1
    FORMATS VST3 AU CLAP Standalone
    PRODUCT_NAME "TalkBox Reference"
    DESCRIPTION "Physically grounded talkbox source-filter emulation"
    IS_SYNTH FALSE
    NEEDS_MIDI_INPUT FALSE
    NEEDS_MIDI_OUTPUT FALSE
    IS_MIDI_EFFECT FALSE
    EDITOR_WANTS_KEYBOARD_FOCUS TRUE
    COPY_PLUGIN_AFTER_BUILD FALSE
    VST3_CATEGORIES Fx
    AU_MAIN_TYPE kAudioUnitType_Effect
    PLUGIN_VERSION 1.0.0)

  target_sources(TalkBoxReference PRIVATE
    src/plugin/PluginProcessor.cpp
    src/plugin/PluginEditor.cpp
    src/plugin/TalkboxApi.cpp)

  target_include_directories(TalkBoxReference PRIVATE src/dsp src/plugin)
  target_compile_definitions(TalkBoxReference PRIVATE
    JUCE_WEB_BROWSER=0
    JUCE_USE_CURL=0
    JUCE_VST3_CAN_REPLACE_VST2=0
    JUCE_DISPLAY_SPLASH_SCREEN=0
    TB_VERSION="1.0.0")

  target_link_libraries(TalkBoxReference PRIVATE
    juce::juce_audio_utils
    juce::juce_audio_processors
    juce::juce_dsp
    juce::juce_gui_extra
  PUBLIC juce::juce_recommended_config_flags juce::juce_recommended_lto_flags)

  set_target_properties(TalkBoxReference PROPERTIES
    CXX_STANDARD 20
    CXX_STANDARD_REQUIRED ON)

  install(TARGETS TalkBoxReference
    BUNDLE DESTINATION . COMPONENT Runtime
    LIBRARY DESTINATION lib COMPONENT Runtime
    RUNTIME DESTINATION bin COMPONENT Runtime)
endif()
