cmake_minimum_required(VERSION 3.20)

# ==============================================================================
# TrainingData — AI training sample collection library
# ==============================================================================

add_library(TrainingDataLib STATIC
    TrainingCollector.cpp
)

target_include_directories(TrainingDataLib PUBLIC ${CMAKE_SOURCE_DIR})
target_compile_features(TrainingDataLib PUBLIC cxx_std_20)

message(STATUS "  TrainingDataLib configured")
