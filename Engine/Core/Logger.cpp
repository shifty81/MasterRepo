#include "Engine/Core/Logger.h"
#include <iostream>

namespace Engine::Core {

Logger::SinkFn Logger::s_sink;

void Logger::SetSink(SinkFn fn) { s_sink = std::move(fn); }

void Logger::Dispatch(const std::string& line) {
    if (s_sink) s_sink(line);
}

void Logger::Init() {
    Info("Logger initialized");
}

void Logger::Info(const std::string& msg) {
    std::string line = "[Info]  " + msg;
    std::cout << line << std::endl;
    Dispatch(line);
}

void Logger::Warn(const std::string& msg) {
    std::string line = "[Warn]  " + msg;
    std::cout << line << std::endl;
    Dispatch(line);
}

void Logger::Error(const std::string& msg) {
    std::string line = "[Error] " + msg;
    std::cerr << line << std::endl;
    Dispatch(line);
}

} // namespace Engine::Core
