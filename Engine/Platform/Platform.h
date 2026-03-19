#pragma once
#include <string>
#include <vector>
#include <cstdint>

namespace Engine::Platform {

// ---------------------------------------------------------------------------
// File system helpers
// ---------------------------------------------------------------------------
bool        FileExists(const std::string& path);
bool        DirectoryExists(const std::string& path);
bool        CreateDirectory(const std::string& path);
std::string ReadTextFile(const std::string& path);
bool        WriteTextFile(const std::string& path, const std::string& content);
std::vector<std::string> ListFiles(const std::string& dir, const std::string& ext = "");
std::string GetExecutablePath();
std::string GetWorkingDirectory();

// ---------------------------------------------------------------------------
// Time
// ---------------------------------------------------------------------------
uint64_t    GetTimeMilliseconds();
std::string GetTimestampISO8601();

// ---------------------------------------------------------------------------
// System
// ---------------------------------------------------------------------------
uint32_t    GetCPUCoreCount();
uint64_t    GetTotalMemoryMB();

} // namespace Engine::Platform
