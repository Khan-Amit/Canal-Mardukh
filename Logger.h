#include "Logger.h"
#include <iostream>
#include <chrono>
#include <iomanip>
#include <ctime>

Logger::Logger()
    : m_IsInitialized(false)
{
}

Logger::~Logger()
{
    Shutdown();
}

bool Logger::Initialize(const std::string& fileName)
{
    if (m_IsInitialized) return true;

    m_LogFile.open(fileName, std::ios::out | std::ios::app);
    if (!m_LogFile.is_open()) {
        std::cerr << "Failed to open log file: " << fileName << "\n";
        return false;
    }

    m_IsInitialized = true;
    Info("Logger initialized.");
    return true;
}

void Logger::Shutdown()
{
    if (!m_IsInitialized) return;

    Info("Logger shutting down.");
    m_LogFile.close();
    m_IsInitialized = false;
}

void Logger::Write(const std::string& message)
{
    if (!m_IsInitialized) return;
    m_LogFile << GetTimeStamp() << " " << message << std::endl;
}

void Logger::Info(const std::string& message)
{
    Write("[INFO] " + message);
}

void Logger::Warning(const std::string& message)
{
    Write("[WARNING] " + message);
}

void Logger::Error(const std::string& message)
{
    Write("[ERROR] " + message);
}

std::string Logger::GetTimeStamp() const
{
    auto now = std::chrono::system_clock::now();
    auto time_t = std::chrono::system_clock::to_time_t(now);
    std::stringstream ss;
    ss << std::put_time(std::localtime(&time_t), "%Y-%m-%d %H:%M:%S");
    return ss.str();
}
