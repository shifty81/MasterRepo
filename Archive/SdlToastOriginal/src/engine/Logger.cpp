#include "Logger.h"

Logger& Logger::Instance() {
    static Logger instance;
    return instance;
}

Logger::~Logger() {
    Shutdown();
}

void Logger::Initialize(const std::string& logFilePath) {
    std::lock_guard<std::mutex> lock(m_mutex);
    if (m_initialized) return;

    m_logFile.open(logFilePath, std::ios::out | std::ios::trunc);
    if (!m_logFile.is_open()) {
        std::cerr << "Logger: failed to open log file: " << logFilePath << std::endl;
    }

    m_errorCount = 0;
    m_warningCount = 0;
    m_initialized = true;

    // Write header
    if (m_logFile.is_open()) {
        m_logFile << "=== Harvest Quest Log ===" << std::endl;
        m_logFile << "Started: " << GetTimestamp() << std::endl;
        m_logFile << "=========================" << std::endl;
    }
}

void Logger::Shutdown() {
    std::lock_guard<std::mutex> lock(m_mutex);
    if (!m_initialized) return;

    if (m_logFile.is_open()) {
        m_logFile << "=========================" << std::endl;
        m_logFile << "Session ended: " << GetTimestamp() << std::endl;
        m_logFile << "Total errors: " << m_errorCount << std::endl;
        m_logFile << "Total warnings: " << m_warningCount << std::endl;
        m_logFile.close();
    }

    m_initialized = false;
}

void Logger::Log(Level level, const std::string& message) {
    std::lock_guard<std::mutex> lock(m_mutex);

    std::string timestamp = GetTimestamp();
    std::string levelStr = LevelToString(level);
    std::string formatted = "[" + timestamp + "] [" + levelStr + "] " + message;

    // Write to console
    if (level == Level::ERROR) {
        std::cerr << formatted << std::endl;
        m_errorCount++;
    } else if (level == Level::WARNING) {
        std::cerr << formatted << std::endl;
        m_warningCount++;
    } else {
        std::cout << formatted << std::endl;
    }

    // Write to log file
    if (m_logFile.is_open()) {
        m_logFile << formatted << std::endl;
        m_logFile.flush();
    }
}

void Logger::Info(const std::string& message) {
    Log(Level::INFO, message);
}

void Logger::Warn(const std::string& message) {
    Log(Level::WARNING, message);
}

void Logger::Error(const std::string& message) {
    Log(Level::ERROR, message);
}

std::string Logger::GetTimestamp() const {
    std::time_t now = std::time(nullptr);
    struct tm timeinfo;
    char buf[64];
#ifdef _WIN32
    localtime_s(&timeinfo, &now);
#else
    localtime_r(&now, &timeinfo);
#endif
    std::strftime(buf, sizeof(buf), "%Y-%m-%d %H:%M:%S", &timeinfo);
    return std::string(buf);
}

std::string Logger::LevelToString(Level level) const {
    switch (level) {
        case Level::INFO:    return "INFO";
        case Level::WARNING: return "WARN";
        case Level::ERROR:   return "ERROR";
        default:             return "UNKNOWN";
    }
}
