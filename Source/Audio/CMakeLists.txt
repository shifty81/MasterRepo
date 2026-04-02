add_library(NFAudio STATIC
    Core/AudioDevice.cpp
    Core/SoundBank.cpp
    Spatial/SpatialAudio.cpp
    Mixer/AudioMixer.cpp
)

add_library(NF::Audio ALIAS NFAudio)

target_include_directories(NFAudio PUBLIC ${CMAKE_SOURCE_DIR}/Source)
target_link_libraries(NFAudio PUBLIC NF::Core)

message(STATUS "  NF::Audio configured")
