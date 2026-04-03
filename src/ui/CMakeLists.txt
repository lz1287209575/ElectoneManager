set(UI_SOURCES
    ElectoneWindow.cpp
    LcdPanel.cpp
    ControlPanel.cpp
    KeyboardPanel.cpp
    FootpadPanel.cpp
    VoiceEditorWindow.cpp
    StepSequencerWindow.cpp
)

add_library(em_ui STATIC ${UI_SOURCES})

target_include_directories(em_ui PUBLIC
    ${CMAKE_SOURCE_DIR}/src
    ${CMAKE_SOURCE_DIR}/src/common
    ${CMAKE_SOURCE_DIR}/src/ui
)

# Use whichever FLTK target was resolved in the parent CMakeLists.txt
target_link_libraries(em_ui PUBLIC ${EM_FLTK_TARGET})

# Note: ScorePanel.h/.cpp are retained in the tree but no longer compiled.
# Lomse dependency is no longer needed (was only used by ScorePanel).
