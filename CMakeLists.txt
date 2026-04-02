cmake_minimum_required(VERSION 3.24)

if(APPLE)
    set(CMAKE_OSX_ARCHITECTURES "arm64;x86_64" CACHE STRING "Build macOS Universal Binaries")
endif()

project(LiveFlowBalancer VERSION 1.0.0 LANGUAGES C CXX)

set(LIVEFLOW_VERSION_SUFFIX "-20260403-beta-1")
set(LIVEFLOW_VERSION_STRING "v${PROJECT_VERSION}${LIVEFLOW_VERSION_SUFFIX}")

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

if (MSVC)
    add_compile_options(/utf-8 /bigobj)
endif ()

option(LIVEFLOW_ENABLE_STANDALONE "Build the standalone app target" OFF)
option(LIVEFLOW_ENABLE_VST3 "Build the VST3 plugin target" ON)
option(LIVEFLOW_ENABLE_AU "Build the AU target on macOS" ON)
option(LIVEFLOW_ENABLE_AAX "Build the AAX target when the SDK is available" OFF)
option(LIVEFLOW_COPY_PLUGIN_AFTER_BUILD "Install the plugin to the default user folder after building" OFF)

include(FetchContent)
FetchContent_Declare(juce
    GIT_REPOSITORY https://github.com/juce-framework/JUCE.git
    GIT_TAG        8.0.12
    GIT_SHALLOW    TRUE
)
FetchContent_MakeAvailable(juce)

if (LIVEFLOW_ENABLE_AAX)
    if (NOT DEFINED AAX_SDK_PATH)
        message(FATAL_ERROR "LIVEFLOW_ENABLE_AAX is ON but AAX_SDK_PATH was not provided.")
    endif ()

    juce_set_aax_sdk_path("${AAX_SDK_PATH}")
endif ()

set(LIVEFLOW_PLUGIN_FORMATS)

if (LIVEFLOW_ENABLE_STANDALONE)
    list(APPEND LIVEFLOW_PLUGIN_FORMATS Standalone)
endif ()

if (LIVEFLOW_ENABLE_VST3)
    list(APPEND LIVEFLOW_PLUGIN_FORMATS VST3)
endif ()

if (APPLE AND LIVEFLOW_ENABLE_AU)
    list(APPEND LIVEFLOW_PLUGIN_FORMATS AU)
endif ()

if (LIVEFLOW_ENABLE_AAX)
    list(APPEND LIVEFLOW_PLUGIN_FORMATS AAX)
endif ()

if (LIVEFLOW_PLUGIN_FORMATS STREQUAL "")
    message(FATAL_ERROR "At least one plugin format must be enabled.")
endif ()

juce_add_binary_data(LiveFlowAssets
    SOURCES
        assets/fonts/SpaceGrotesk-Variable.ttf)

add_library(liveflow_core STATIC
    src/core/BalancerEngine.h
    src/core/BalancerMath.h
    src/core/LookAheadDelay.h
    src/core/LoudnessEstimator.h
    src/core/PluginParameters.cpp
    src/core/PluginParameters.h
    src/core/VisualizationState.cpp
    src/core/VisualizationState.h)

target_include_directories(liveflow_core
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/src)

target_link_libraries(liveflow_core
    PUBLIC
        juce::juce_audio_processors
        juce::juce_dsp)

target_compile_definitions(liveflow_core
    PUBLIC
        JUCE_WEB_BROWSER=0
        JUCE_USE_CURL=0
        JUCE_VST3_CAN_REPLACE_VST2=0
        DONT_SET_USING_JUCE_NAMESPACE=1
        LIVEFLOW_VERSION=\"${PROJECT_VERSION}\")

function(configure_liveflow_plugin_target target_name)
    target_include_directories(${target_name}
        PRIVATE
            ${CMAKE_CURRENT_SOURCE_DIR}/src
            ${CMAKE_CURRENT_SOURCE_DIR}/external/JUCE/examples/Plugins)

    target_link_libraries(${target_name}
        PRIVATE
            LiveFlowAssets
            juce::juce_audio_utils
        PUBLIC
            juce::juce_recommended_config_flags
            juce::juce_recommended_lto_flags
            juce::juce_recommended_warning_flags)

    target_compile_definitions(${target_name}
        PUBLIC
            JUCE_WEB_BROWSER=0
            JUCE_USE_CURL=0
            JUCE_VST3_CAN_REPLACE_VST2=0
            DONT_SET_USING_JUCE_NAMESPACE=1
            LIVEFLOW_VERSION_EXT="${LIVEFLOW_VERSION_STRING}")
endfunction()

juce_add_plugin(LiveFlowBalancer
    COMPANY_NAME "LiveFlow"
    COMPANY_WEBSITE "https://example.com/liveflow"
    COMPANY_EMAIL "support@example.com"
    IS_SYNTH FALSE
    NEEDS_MIDI_INPUT FALSE
    NEEDS_MIDI_OUTPUT FALSE
    IS_MIDI_EFFECT FALSE
    EDITOR_WANTS_KEYBOARD_FOCUS FALSE
    COPY_PLUGIN_AFTER_BUILD ${LIVEFLOW_COPY_PLUGIN_AFTER_BUILD}
    PLUGIN_MANUFACTURER_CODE Lflo
    PLUGIN_CODE Lfb1
    VST3_CATEGORIES Fx Dynamics
    FORMATS ${LIVEFLOW_PLUGIN_FORMATS}
    VERSION ${PROJECT_VERSION}
    PRODUCT_NAME "LiveFlow Balancer"
    PLUGIN_NAME "LiveFlow Balancer")

target_sources(LiveFlowBalancer
    PRIVATE
        src/plugin/LiveFlowAudioProcessor.cpp
        src/plugin/LiveFlowAudioProcessor.h
        src/plugin/LiveFlowAudioProcessorEditor.cpp
        src/plugin/LiveFlowAudioProcessorEditor.h
        src/ui/CompactKnob.h
        src/ui/GainTimelineComponent.h
        src/ui/LiveFlowLookAndFeel.cpp
        src/ui/LiveFlowLookAndFeel.h
        src/ui/PresenceIndicator.h
        src/ui/HelpOverlay.cpp
        src/ui/HelpOverlay.h
        src/ui/AboutOverlay.cpp
        src/ui/AboutOverlay.h
        src/ui/Translations.h
        src/ui/VisualizerComponent.cpp
        src/ui/VisualizerComponent.h)

target_link_libraries(LiveFlowBalancer
    PRIVATE
        liveflow_core)

configure_liveflow_plugin_target(LiveFlowBalancer)

include(CTest)

if (BUILD_TESTING)
    juce_add_console_app(LiveFlowTests)

    target_sources(LiveFlowTests
        PRIVATE
            tests/LiveFlowTests.cpp)

    target_link_libraries(LiveFlowTests
        PRIVATE
            liveflow_core
        PUBLIC
            juce::juce_recommended_config_flags
            juce::juce_recommended_warning_flags)

    add_test(NAME liveflow_core_tests COMMAND LiveFlowTests)
endif ()

# ==============================================================================
# Inbuilt Windows Installer (CPack NSIS)
# ==============================================================================
if (WIN32)
    # Define what goes into the installer
    install(DIRECTORY "${CMAKE_BINARY_DIR}/LiveFlowBalancer_artefacts/Release/VST3/LiveFlow Balancer.vst3"
            DESTINATION "."
            COMPONENT VST3)
            

    # CPack variables
    set(CPACK_GENERATOR "NSIS")
    set(CPACK_PACKAGE_NAME "LiveFlow Balancer")
    set(CPACK_PACKAGE_VENDOR "Zilin Wang")
    set(CPACK_PACKAGE_CONTACT "zilinww@gmail.com")
    set(CPACK_PACKAGE_VERSION "${LIVEFLOW_VERSION_STRING}")
    set(CPACK_PACKAGE_INSTALL_DIRECTORY "Common Files\\\\VST3\\\\LiveFlow Balancer")
    set(CPACK_PACKAGE_INSTALL_REGISTRY_KEY "LiveFlow Balancer")
    
    # NSIS specific configuration for Control Panel Uninstall entries
    set(CPACK_NSIS_DISPLAY_NAME "LiveFlow Balancer")
    set(CPACK_RESOURCE_FILE_LICENSE "${CMAKE_CURRENT_SOURCE_DIR}/LICENSE.txt")
    set(CPACK_NSIS_URL_INFO_ABOUT "mailto:zilinww@gmail.com")
    set(CPACK_NSIS_HELP_LINK "mailto:zilinww@gmail.com")
    set(CPACK_NSIS_CONTACT "zilinww@gmail.com")
    set(CPACK_NSIS_MODIFY_PATH OFF)
    set(CPACK_NSIS_ENABLE_UNINSTALL_BEFORE_INSTALL ON)
    
    # 强制将 NSIS 自动安装包界面的系统语言强硬锁定为简体中文
    set(CPACK_NSIS_LANGUAGE "SimpChinese")
    
    # Exclude Unspecified targets (like juceaide or testing utilities)
    set(CPACK_COMPONENTS_ALL VST3)
    
    include(CPack)
endif ()
