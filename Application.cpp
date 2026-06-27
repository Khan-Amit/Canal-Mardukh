#ifndef CANAL_MARDUKH_APPLICATION_H
#define CANAL_MARDUKH_APPLICATION_H

#include <string>
#include <memory>

class PoolManager;

class Application
{
public:
    Application();
    ~Application();

    bool Initialize();
    void Run();
    void Shutdown();

private:
    std::string m_Name;
    std::string m_Version;
    bool m_IsRunning;

    std::unique_ptr<PoolManager> m_PoolManager;
};

#endif
