#include "Logger.h"

#include <iostream>
#include <chrono>
#include <iomanip>
#include <sstream>

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
    m_LogFile.open(fileName, std::ios::out | std::ios::app);

    if (!m_LogFile.is_open())
    {
        return false;
    }

    m_IsInitialized = true;

    Info("Logger Initialized.");

    return true;
}

void Logger::Shutdown()
{
    if (!m_IsInitialized)
        return;

    Info("Logger Shutdown.");

    m_LogFile.close();

    m_IsInitialized = false;
}

void Logger::Write(const std::string& message)
{
    if (!m_IsInitialized)
        return;

    std::string line = GetTimeStamp() + " " + message;

    std::cout << line << std::endl;

    m_LogFile << line << std::endl;
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

    auto time = std::chrono::system_clock::to_time_t(now);

    std::tm localTime{};

#ifdef _WIN32
    localtime_s(&localTime, &time);
#else
    localtime_r(&time, &localTime);
#endif

    std::ostringstream stream;

    stream << "["
           << std::put_time(&localTime, "%Y-%m-%d %H:%M:%S")
           << "]";

    return stream.str();
}
