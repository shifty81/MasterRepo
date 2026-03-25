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
    # Phase 25 — code snippet library
    SnippetManager/SnippetManager.cpp
    # Phase 26 — TODO/FIXME task panel
    TaskPanel/TaskPanel.cpp
    # Phase 27 — variable watch list panel
    WatchPanel/WatchPanel.cpp
    # Phase 28 — build/run output log panel
    OutputPanel/OutputPanel/OutputPanel.cpp
    # Phase 29 — IDE breakpoint management
    Debugger/BreakpointManager/BreakpointManager.cpp
    # Phase 30 — LSP diagnostic message manager
    LSP/LSPDiagnostics/LSPDiagnostics.cpp
    # Phase 31 — code search engine with indexing and async search
    Search/CodeSearchEngine/CodeSearchEngine.cpp
    # Phase 32 — code refactoring panel (rename, extract, move)
    Refactor/RefactorPanel/RefactorPanel.cpp
    # Phase 33 — console log filter with regex/text support
    Console/ConsoleFilter/ConsoleFilter.cpp
    # Phase 34 — file-path navigation breadcrumb bar
    Breadcrumb/BreadcrumbBar/BreadcrumbBar.cpp
)

add_library(IDELib STATIC ${IDE_SOURCES})

target_include_directories(IDELib PUBLIC ${CMAKE_SOURCE_DIR})
target_compile_features(IDELib PUBLIC cxx_std_20)

message(STATUS "  IDELib configured with ${CMAKE_CURRENT_SOURCE_DIR}")
