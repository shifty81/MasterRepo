/// @file LogTests.cpp — Unit tests for the Core logging system.
#include <catch2/catch_test_macros.hpp>
#include "Core/Logging/Log.h"

using namespace NF;

TEST_CASE("LogLevel ordering", "[core][log]") {
    REQUIRE(static_cast<int>(LogLevel::Trace)   < static_cast<int>(LogLevel::Debug));
    REQUIRE(static_cast<int>(LogLevel::Debug)   < static_cast<int>(LogLevel::Info));
    REQUIRE(static_cast<int>(LogLevel::Info)    < static_cast<int>(LogLevel::Warning));
    REQUIRE(static_cast<int>(LogLevel::Warning) < static_cast<int>(LogLevel::Error));
    REQUIRE(static_cast<int>(LogLevel::Error)   < static_cast<int>(LogLevel::Fatal));
}

TEST_CASE("Logger::Log does not throw", "[core][log]") {
    REQUIRE_NOTHROW(Logger::Log(LogLevel::Info,    "Test", "info message"));
    REQUIRE_NOTHROW(Logger::Log(LogLevel::Warning, "Test", "warning message"));
    REQUIRE_NOTHROW(Logger::Log(LogLevel::Error,   "Test", "error message"));
}

TEST_CASE("NF_LOG macros compile and run without throwing", "[core][log]") {
    REQUIRE_NOTHROW(NF_LOG_INFO("Test", "macro info"));
    REQUIRE_NOTHROW(NF_LOG_WARN("Test", "macro warn"));
    REQUIRE_NOTHROW(NF_LOG_ERROR("Test", "macro error"));
}
