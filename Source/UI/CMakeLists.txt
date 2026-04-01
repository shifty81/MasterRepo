add_library(NFUI STATIC
    Framework/Widget.cpp
    Framework/WidgetTree.cpp
    Widgets/Button.cpp
    Widgets/TreeView.cpp
    Widgets/TextInput.cpp
    Rendering/UIRenderer.cpp
)

add_library(NF::UI ALIAS NFUI)

target_include_directories(NFUI PUBLIC
    ${CMAKE_SOURCE_DIR}/Source
    ${CMAKE_SOURCE_DIR}/ThirdParty
)

target_link_libraries(NFUI PUBLIC NF::Core NF::Renderer)

message(STATUS "  NF::UI configured")
