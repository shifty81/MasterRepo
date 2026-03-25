#pragma once
#include <functional>
#include <string>

namespace Engine::Core {

class Logger {
public:
    static void Init();
    static void Info(const std::string& msg);
    static void Warn(const std::string& msg);
    static void Error(const std::string& msg);

    // EI-01: Register a sink callback that receives every log line.
    // Pass nullptr to remove the sink.
    using SinkFn = std::function<void(const std::string&)>;
    static void SetSink(SinkFn fn);

private:
    static SinkFn s_sink;

    static void Dispatch(const std::string& line);
};

} // namespace Engine::Core
