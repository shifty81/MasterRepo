find_package(OpenGL QUIET)

# GLFW discovery with FetchContent fallback
set(GLFW_LIBS "")
if(OpenGL_FOUND)
    find_package(glfw3 3.3 QUIET)
    if(glfw3_FOUND)
        set(GLFW_LIBS glfw)
        message(STATUS "  GLFW: found via find_package")
    else()
        find_library(GLFW_LIB NAMES glfw glfw3 QUIET)
        if(GLFW_LIB)
            set(GLFW_LIBS ${GLFW_LIB})
            message(STATUS "  GLFW: found via find_library -> ${GLFW_LIB}")
        else()
            include(FetchContent)
            FetchContent_Declare(glfw
                GIT_REPOSITORY https://github.com/glfw/glfw.git
                GIT_TAG        3.4
                GIT_SHALLOW    TRUE
            )
            set(GLFW_BUILD_DOCS OFF CACHE BOOL "" FORCE)
            set(GLFW_BUILD_TESTS OFF CACHE BOOL "" FORCE)
            set(GLFW_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
            set(GLFW_INSTALL OFF CACHE BOOL "" FORCE)
            FetchContent_MakeAvailable(glfw)
            set(GLFW_LIBS glfw)
            message(STATUS "  GLFW: built from source via FetchContent")
        endif()
    endif()
else()
    message(STATUS "  OpenGL not found — renderer will compile as stub")
endif()

add_library(NFRenderer STATIC
    RHI/RenderDevice.cpp
    RHI/Shader.cpp
    RHI/Texture.cpp
    Pipeline/ForwardRenderer.cpp
    Materials/Material.cpp
    Lighting/LightSystem.cpp
    Mesh/MeshRenderer.cpp
    Debug/DebugDraw.cpp
    PostProcess/PostProcessStack.cpp
)

add_library(NF::Renderer ALIAS NFRenderer)

target_include_directories(NFRenderer PUBLIC
    ${CMAKE_SOURCE_DIR}/Source
    ${CMAKE_SOURCE_DIR}/ThirdParty
)

target_link_libraries(NFRenderer PUBLIC
    NF::Core
    NF::Engine
)

if(OpenGL_FOUND)
    target_compile_definitions(NFRenderer PUBLIC NF_HAS_OPENGL=1)
    target_link_libraries(NFRenderer PUBLIC OpenGL::GL)
    if(GLFW_LIBS)
        target_link_libraries(NFRenderer PUBLIC ${GLFW_LIBS})
    endif()
endif()

# Link X11/pthread on Linux
if(UNIX AND NOT APPLE)
    find_package(Threads REQUIRED)
    find_package(X11 QUIET)
    target_link_libraries(NFRenderer PUBLIC Threads::Threads)
    if(X11_FOUND)
        target_link_libraries(NFRenderer PUBLIC ${X11_LIBRARIES})
    endif()
endif()

message(STATUS "  NF::Renderer configured")
