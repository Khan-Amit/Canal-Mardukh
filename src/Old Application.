#include "Application.h"

#include <iostream>

Application::Application()
    : m_Name("Canal-Mardukh"),
      m_Version("0.1.0"),
      m_IsRunning(false)
{
}

Application::~Application()
{
}

bool Application::Initialize()
{
    std::cout << "\n=========================================\n";
    std::cout << "Initializing " << m_Name << "\n";
    std::cout << "Version : " << m_Version << "\n";
    std::cout << "=========================================\n\n";

    m_IsRunning = true;

    return true;
}

void Application::Run()
{
    if (!m_IsRunning)
        return;

    std::cout << "Application Running...\n";
    std::cout << "Dashboard Loading...\n";
    std::cout << "Waiting for Start command...\n";
}

void Application::Shutdown()
{
    std::cout << "\nShutting Down Canal-Mardukh...\n";

    m_IsRunning = false;

    std::cout << "Shutdown Complete.\n";
}
