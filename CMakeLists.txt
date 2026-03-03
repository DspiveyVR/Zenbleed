cmake_minimum_required(VERSION 3.25)
list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")
include (PamplejuceVersion)

# Modern concise way to add dependencies to your project
include (CPM)

# Configures universal binaries and decides which version of macOS to support
include(PamplejuceMacOS)

# Couple tweaks that IMO should be JUCE defaults
include(JUCEDefaults)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Prevent submodules from installing
set(JUCE_INSTALL_DESTINATION "lib/cmake/JUCE" CACHE STRING "" FORCE)
set(CMAKE_SKIP_INSTALL_ALL_DEPENDENCY TRUE)

set(PROJECT_NAME "Zenbleed")

# Worry not, JUCE's PRODUCT_NAME can have spaces (and is what DAWs will display)
# You can also just have it be the same thing as PROJECT_NAME
# You may want to append the major version on the end of this (and PROJECT_NAME) ala:
#   set(PROJECT_NAME "MyPlugin_v${MAJOR_VERSION}")
# Doing so enables major versions to show up in IDEs and DAWs as separate plugins
# allowing you to change parameters and behavior without breaking existing user projects
set(PRODUCT_NAME "Zenbleed")
set(COMPANY_NAME "CSCI591USCA")
set(BUNDLE_ID "io.github.CSCI591USCA.zenbleed")
set(FORMATS VST3)

# Force JUCE to work with sccache
# instruct MSVC to embed debug info (/Z7) instead of emitting a .pdb
set(CMAKE_POLICY_DEFAULT_CMP0141        NEW      CACHE STRING "" FORCE)
set(CMAKE_MSVC_DEBUG_INFORMATION_FORMAT Embedded CACHE STRING "" FORCE)

# For simplicity, the name of the CMake project is also the name of the target
project(${PROJECT_NAME} VERSION ${CURRENT_VERSION})

# JUCE is setup as a submodule in the /JUCE folder
# Locally, you must run `git submodule update --init --recursive` once
# and later `git submodule update --remote --merge` to keep it up to date
# On Github Actions, this is done as a part of actions/checkout
add_subdirectory(JUCE EXCLUDE_FROM_ALL)

# Add any other modules you want modules here, before the juce_add_plugin call
# juce_add_module(modules/mymodule)

# This adds the melatonin inspector module
add_subdirectory(modules/melatonin_inspector)

# See `docs/CMake API.md` in the JUCE repo for all config options
juce_add_plugin("${PROJECT_NAME}"
    NEEDS_MIDI_INPUT TRUE
    NEEDS_MIDI_OUTPUT TRUE
    IS_MIDI_EFFECT TRUE
    IS_SYNTH TRUE

    # Icons for the standalone app
    ICON_BIG "${CMAKE_CURRENT_SOURCE_DIR}/packaging/icon.png"

    COMPANY_NAME "${COMPANY_NAME}"
    BUNDLE_ID "${BUNDLE_ID}"

    # On MacOS, plugin is copied to /Users/yourname/Library/Audio/Plug-Ins/
    COPY_PLUGIN_AFTER_BUILD FALSE
    PLUGIN_MANUFACTURER_CODE Znbl
    PLUGIN_CODE Z001
    FORMATS "${FORMATS}"
    PRODUCT_NAME "${PRODUCT_NAME}")

# This lets us use our code in both the JUCE targets and our Test target
# Without running into ODR violations
add_library(SharedCode INTERFACE)

# Enable fast math, C++20 and a few other target defaults
include(SharedCodeDefaults)

# Manually list all .h and .cpp files for the plugin
# If you are like me, you'll use globs for your sanity.
# Just ensure you employ CONFIGURE_DEPENDS so the build system picks up changes
# If you want to appease the CMake gods and avoid globs, manually add files like so:
# set(SourceFiles Source/PluginEditor.h Source/PluginProcessor.h Source/PluginEditor.cpp Source/PluginProcessor.cpp)
file(GLOB_RECURSE SourceFiles CONFIGURE_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/source/*.cpp" "${CMAKE_CURRENT_SOURCE_DIR}/source/*.h")
target_sources(SharedCode INTERFACE ${SourceFiles})

# Adds a BinaryData target for embedding assets into the binary
include(Assets)

# MacOS only: Cleans up folder and target organization on Xcode.
include(XcodePrettify)

# This is where you can set preprocessor definitions for JUCE and your plugin
target_compile_definitions(SharedCode
    INTERFACE

    JUCE_WEB_BROWSER=0
    JUCE_USE_CURL=0
    JUCE_VST3_CAN_REPLACE_VST2=0

    # Uncomment if you are paying for a an Indie/Pro license or releasing under GPLv3
    # JUCE_DISPLAY_SPLASH_SCREEN=0
    CMAKE_BUILD_TYPE="${CMAKE_BUILD_TYPE}"
    VERSION="${CURRENT_VERSION}"
    PRODUCT_NAME_WITHOUT_VERSION="${PRODUCT_NAME}"
)

# Link to any other modules you added (with juce_add_module) here!
# Usually JUCE modules must have PRIVATE visibility
# See https://github.com/juce-framework/JUCE/blob/master/docs/CMake%20API.md#juce_add_module
# However, with Pamplejuce, you'll link modules to SharedCode with INTERFACE visibility
# This allows the JUCE plugin targets and the Tests target to link against it
target_link_libraries(SharedCode
    INTERFACE
    Assets
    melatonin_inspector
    juce_audio_utils
    juce_audio_processors
    juce_dsp
    juce_gui_basics
    juce_gui_extra
    juce_events
    juce_audio_formats
    juce_audio_devices
    juce::juce_recommended_config_flags
    juce::juce_recommended_lto_flags
    juce::juce_recommended_warning_flags)

# Link the JUCE plugin targets our SharedCode target
target_link_libraries("${PROJECT_NAME}" PRIVATE SharedCode Assets)

if(WIN32)
    set(VST3_INSTALL_DIR "$ENV{LOCALAPPDATA}/Programs/Common Files/VST3")
elseif(APPLE)
    set(VST3_INSTALL_DIR "$ENV{HOME}/Library/Audio/Plug-Ins/VST3")
else()
    set(VST3_INSTALL_DIR "$ENV{HOME}/.vst3")
endif()

install(DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}_artefacts/VST3/${PRODUCT_NAME}.vst3"
    DESTINATION "${VST3_INSTALL_DIR}"
    USE_SOURCE_PERMISSIONS
    COMPONENT plugin)

add_custom_target(install-plugin
    COMMAND ${CMAKE_COMMAND} --install ${CMAKE_BINARY_DIR} --component plugin
    COMMENT "Installing VST3 plugin to ${VST3_INSTALL_DIR}"
)

# IPP support, comment out to disable
include(PamplejuceIPP)

# Everything related to the tests target
include(Tests)

# A separate target for Benchmarks (keeps the Tests target fast)
include(Benchmarks)

# Output some config for CI (like our PRODUCT_NAME)
include(GitHubENV)

# Doxygen awesome
include(FetchContent)
FetchContent_Declare(
    doxygen-awesome-css
    URL https://github.com/jothepro/doxygen-awesome-css/archive/refs/heads/main.zip
)
FetchContent_MakeAvailable(doxygen-awesome-css)
 
# Save the location the files were cloned into
# This allows us to get the path to doxygen-awesome.css
FetchContent_GetProperties(doxygen-awesome-css SOURCE_DIR AWESOME_CSS_DIR)
 
# Generate the Doxyfile
set(DOXYFILE_IN ${CMAKE_CURRENT_SOURCE_DIR}/doc/Doxyfile.in)
set(DOXYFILE_OUT ${CMAKE_CURRENT_BINARY_DIR}/Doxyfile)
configure_file(${DOXYFILE_IN} ${DOXYFILE_OUT} @ONLY)
