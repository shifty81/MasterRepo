cmake_minimum_required(VERSION 3.20)

# ==============================================================================
# Phase 8: IDE & Integrated Tooling
# ==============================================================================

set(IDE_SOURCES
    CodeEditor/CodeEditor.cpp
    AIChat/AIChat.cpp
    Console/IDEConsole.cpp
    Debugger/Debugger.cpp
    LSP/LSPClient.cpp
    NLAssistant/NLAssistant.cpp
    InteractiveDocs/InteractiveDocs.cpp
    # Phase 19 — voice-to-action pipeline
    VoiceInput/VoiceInput.cpp
    # Phase 24 — project file tree explorer
    ProjectExplorer/ProjectExplorer.cpp
)

add_library(IDELib STATIC ${IDE_SOURCES})

target_include_directories(IDELib PUBLIC ${CMAKE_SOURCE_DIR})
target_compile_features(IDELib PUBLIC cxx_std_20)

message(STATUS "  IDELib configured with ${CMAKE_CURRENT_SOURCE_DIR}")
