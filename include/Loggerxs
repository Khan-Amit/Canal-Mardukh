#ifndef CANAL_MARDUKH_LOGGER_H
#define CANAL_MARDUKH_LOGGER_H

#include <string>
#include <fstream>

class Logger
{
public:

    Logger();

    ~Logger();

    bool Initialize(const std::string& fileName);

    void Shutdown();

    void Write(const std::string& message);

    void Info(const std::string& message);

    void Warning(const std::string& message);

    void Error(const std::string& message);

private:

    std::string GetTimeStamp() const;

private:

    std::ofstream m_LogFile;

    bool m_IsInitialized;
};

#endif
