cmake_minimum_required(VERSION 3.21)
project(ElectoneManager VERSION 1.0.0 LANGUAGES CXX OBJCXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# ──────────────────────────────────────────────
# Build options
# ──────────────────────────────────────────────
set(EM_BUFFER_SIZE 256 CACHE STRING "Audio buffer size in samples")
set(EM_SAMPLE_RATE 48000 CACHE STRING "Target sample rate")
option(EM_FLUTTER "Build for Flutter FFI consumption" OFF)

add_compile_definitions(
    EM_BUFFER_SIZE=${EM_BUFFER_SIZE}
    EM_SAMPLE_RATE=${EM_SAMPLE_RATE}
)

if(EM_FLUTTER)
    add_compile_definitions(EM_FLUTTER=1)
endif()

# ──────────────────────────────────────────────
# Optimization flags (non-MSVC)
# ──────────────────────────────────────────────
if(NOT MSVC)
    add_compile_options(-O2 -Wall -Wextra)
    if(NOT APPLE AND NOT ANDROID)
        # Only safe on x86 Linux / Windows (not cross-compiling ARM)
        add_compile_options(-march=native)
    endif()
else()
    add_compile_options(/O2 /W3)
endif()

# ──────────────────────────────────────────────
# Common source files
# ──────────────────────────────────────────────
set(COMMON_SOURCES
    src/common/MidiToChineseInstrumentMapper.cpp
    src/common/SamplerEngine.cpp
    src/common/DrumKitEngine.cpp
    src/common/MidiFileParser.cpp
)

set(COMMON_HEADERS
    src/common/IMidiDevice.h
    src/common/IAudioEngine.h
    src/common/IMidiOutput.h
    src/common/MidiMessage.h
    src/common/RingBuffer.h
    src/common/AudioDeviceScanner.h
    src/common/MidiToChineseInstrumentMapper.h
    src/common/SamplerEngine.h
    src/common/TransportEngine.h
    src/common/DrumPattern.h
    src/common/DrumPatternBank.h
    src/common/DrumSequencer.h
    src/common/DrumKitEngine.h
    src/common/MidiFileParser.h
    src/common/MidiFilePlayer.h
)

# ──────────────────────────────────────────────
# Platform-specific sources & libraries
# ──────────────────────────────────────────────
set(PLATFORM_SOURCES "")
set(PLATFORM_LIBS "")
set(PLATFORM_FRAMEWORKS "")

if(APPLE)
    list(APPEND PLATFORM_SOURCES
        src/platform/apple/CoreMidiDevice.cpp
        src/platform/apple/CoreAudioEngine.mm
        src/platform/apple/CoreMidiOutput.cpp
    )
    # Link Apple frameworks
    find_library(CORE_AUDIO_LIB      CoreAudio      REQUIRED)
    find_library(CORE_MIDI_LIB       CoreMIDI       REQUIRED)
    find_library(AUDIO_TOOLBOX_LIB   AudioToolbox   REQUIRED)
    find_library(CORE_FOUNDATION_LIB CoreFoundation REQUIRED)
    list(APPEND PLATFORM_LIBS
        ${CORE_AUDIO_LIB}
        ${CORE_MIDI_LIB}
        ${AUDIO_TOOLBOX_LIB}
        ${CORE_FOUNDATION_LIB}
    )
    if(IOS)
        find_library(AV_FOUNDATION_LIB AVFoundation REQUIRED)
        list(APPEND PLATFORM_LIBS ${AV_FOUNDATION_LIB})
        add_compile_definitions(EM_IOS=1)
    else()
        add_compile_definitions(EM_MACOS=1)
    endif()

elseif(WIN32)
    list(APPEND PLATFORM_SOURCES
        src/platform/windows/WinMidiDevice.cpp
        src/platform/windows/AsioAudioEngine.cpp
        src/platform/windows/WasapiAudioEngine.cpp
        src/platform/windows/WinMidiOutput.cpp
    )
    list(APPEND PLATFORM_LIBS
        winmm
        ole32
        uuid
        ksuser
    )
    add_compile_definitions(EM_WINDOWS=1 WIN32_LEAN_AND_MEAN NOMINMAX)

elseif(ANDROID)
    list(APPEND PLATFORM_SOURCES
        src/platform/android/AndroidMidiDevice.cpp
        src/platform/android/OboeAudioEngine.cpp
        src/platform/android/AndroidMidiOutput.cpp
    )
    # Oboe: either as submodule or find_package
    find_package(oboe QUIET)
    if(oboe_FOUND)
        list(APPEND PLATFORM_LIBS oboe::oboe)
    else()
        message(WARNING "Oboe not found via find_package. "
                        "Add Oboe as a subdirectory or set OBOE_DIR.")
        # Fallback: submodule at third_party/oboe
        if(EXISTS "${CMAKE_SOURCE_DIR}/third_party/oboe/CMakeLists.txt")
            add_subdirectory(third_party/oboe)
            list(APPEND PLATFORM_LIBS oboe)
        endif()
    endif()
    list(APPEND PLATFORM_LIBS log android amidi)
    add_compile_definitions(EM_ANDROID=1)

elseif(UNIX)  # Linux
    list(APPEND PLATFORM_SOURCES
        src/platform/linux/AlsaMidiDevice.cpp
        src/platform/linux/AlsaAudioEngine.cpp
        src/platform/linux/AlsaMidiOutput.cpp
    )
    find_package(ALSA REQUIRED)
    list(APPEND PLATFORM_LIBS ${ALSA_LIBRARIES})
    include_directories(${ALSA_INCLUDE_DIRS})
    add_compile_definitions(EM_LINUX=1)
endif()

# ──────────────────────────────────────────────
# UI layer (FLTK) — desktop platforms only
# ──────────────────────────────────────────────
option(EM_ENABLE_UI "Build with FLTK UI" ON)

if(EM_ENABLE_UI AND NOT ANDROID AND NOT IOS)
    # Try CONFIG mode first (FLTK 1.4+), then module mode (FLTK 1.3)
    find_package(FLTK QUIET CONFIG)
    if(NOT FLTK_FOUND)
        find_package(FLTK QUIET)
    endif()

    # Resolve whichever target name Homebrew/system provides
    if(TARGET fltk::fltk)
        set(EM_FLTK_TARGET fltk::fltk)
    elseif(TARGET fltk)
        set(EM_FLTK_TARGET fltk)
    elseif(FLTK_FOUND AND FLTK_LIBRARIES)
        # Module mode: create an imported target for consistency
        add_library(em_fltk_imported INTERFACE IMPORTED)
        target_link_libraries(em_fltk_imported INTERFACE ${FLTK_LIBRARIES})
        target_include_directories(em_fltk_imported INTERFACE ${FLTK_INCLUDE_DIR})
        set(EM_FLTK_TARGET em_fltk_imported)
    endif()

    if(EM_FLTK_TARGET)
        add_subdirectory(src/ui)
        add_compile_definitions(EM_UI_ENABLED=1)
        message(STATUS "  FLTK found (${EM_FLTK_TARGET}): UI enabled")
    else()
        message(WARNING "  FLTK not found — building headless (no UI).")
        set(EM_ENABLE_UI OFF)
    endif()
endif()

# ──────────────────────────────────────────────
# Flutter shared library target (macOS dylib / Android .so)
# ──────────────────────────────────────────────
if(EM_FLUTTER AND NOT IOS)
    add_library(em_engine_shared SHARED
        ${COMMON_SOURCES}
        ${PLATFORM_SOURCES}
        src/ios/em_bridge.cpp
    )
    target_include_directories(em_engine_shared PUBLIC
        ${CMAKE_SOURCE_DIR}/src
        ${CMAKE_SOURCE_DIR}/src/common
        ${CMAKE_SOURCE_DIR}/src/platform
        ${CMAKE_SOURCE_DIR}/src/ios
    )
    target_link_libraries(em_engine_shared PUBLIC ${PLATFORM_LIBS})
    set_target_properties(em_engine_shared PROPERTIES
        OUTPUT_NAME "em_engine"
        LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}"
    )
    if(APPLE)
        set_target_properties(em_engine_shared PROPERTIES
            INSTALL_RPATH "@loader_path"
        )
    endif()
    install(TARGETS em_engine_shared LIBRARY DESTINATION lib)
    install(FILES src/ios/em_bridge.h DESTINATION include)
endif()

# ──────────────────────────────────────────────
# iOS static library target (for Swift/Flutter iOS)
# ──────────────────────────────────────────────
if(IOS)
    add_library(em_engine STATIC
        ${COMMON_SOURCES}
        ${PLATFORM_SOURCES}
        src/ios/em_bridge.cpp
    )
    target_include_directories(em_engine PUBLIC
        ${CMAKE_SOURCE_DIR}/src
        ${CMAKE_SOURCE_DIR}/src/common
        ${CMAKE_SOURCE_DIR}/src/platform
        ${CMAKE_SOURCE_DIR}/src/ios
    )
    target_link_libraries(em_engine PUBLIC ${PLATFORM_LIBS})
    install(TARGETS em_engine ARCHIVE DESTINATION lib)
    install(FILES src/ios/em_bridge.h DESTINATION include)
endif()

# ──────────────────────────────────────────────
# Executable target
# ──────────────────────────────────────────────
if(APPLE AND NOT IOS)
    # Package as a proper .app bundle so macOS launches it without a Terminal window
    add_executable(ElectoneManager MACOSX_BUNDLE
        src/main.cpp
        ${COMMON_SOURCES}
        ${PLATFORM_SOURCES}
    )
    set_target_properties(ElectoneManager PROPERTIES
        MACOSX_BUNDLE_INFO_PLIST "${CMAKE_SOURCE_DIR}/config/Info.macos.plist"
    )
else()
    add_executable(ElectoneManager
        src/main.cpp
        ${COMMON_SOURCES}
        ${PLATFORM_SOURCES}
    )
endif()

target_include_directories(ElectoneManager PRIVATE
    ${CMAKE_SOURCE_DIR}/src
    ${CMAKE_SOURCE_DIR}/src/common
    ${CMAKE_SOURCE_DIR}/src/platform
    ${CMAKE_SOURCE_DIR}/src/ui
)

target_link_libraries(ElectoneManager PRIVATE
    ${PLATFORM_LIBS}
)

if(EM_ENABLE_UI AND TARGET em_ui)
    target_link_libraries(ElectoneManager PRIVATE em_ui)
endif()

# ──────────────────────────────────────────────
# Install rules
# ──────────────────────────────────────────────
install(TARGETS ElectoneManager DESTINATION bin)

# ──────────────────────────────────────────────
# Print config summary
# ──────────────────────────────────────────────
message(STATUS "=== ElectoneManager Build Configuration ===")
message(STATUS "  Platform    : ${CMAKE_SYSTEM_NAME}")
message(STATUS "  C++ std     : C++${CMAKE_CXX_STANDARD}")
message(STATUS "  Buffer size : ${EM_BUFFER_SIZE} samples")
message(STATUS "  Sample rate : ${EM_SAMPLE_RATE} Hz")
message(STATUS "  Target delay: ~${EM_BUFFER_SIZE}/${EM_SAMPLE_RATE}*1000 ms")
message(STATUS "===========================================")
