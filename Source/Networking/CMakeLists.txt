add_library(NFNetworking STATIC
    Transport/Socket.cpp
    Session/SessionManager.cpp
    HTTP/HttpClient.cpp

    # Phase 6: Multiplayer protocol and channel
    Protocol/NetMessage.cpp
    Channel/NetChannel.cpp
)

add_library(NF::Networking ALIAS NFNetworking)

target_include_directories(NFNetworking PUBLIC ${CMAKE_SOURCE_DIR}/Source)
target_link_libraries(NFNetworking PUBLIC NF::Core)

message(STATUS "  NF::Networking configured")
