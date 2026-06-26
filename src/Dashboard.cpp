#include "Dashboard.h"

#include <iostream>

Dashboard::Dashboard()
    : m_Wallet(""),
      m_SelectedPool("None"),
      m_Hashrate(0.0),
      m_Temperature(0.0),
      m_Runtime("00:00:00"),
      m_Submitted(0),
      m_Accepted(0),
      m_Rejected(0),
      m_EstimatedReward(0.0)
{
}

Dashboard::~Dashboard()
{
}

void Dashboard::Initialize()
{
    std::cout << "Dashboard Initialized.\n";
}

void Dashboard::Draw()
{
    std::cout << "\n=========================================\n";
    std::cout << "          Canal-Mardukh™\n";
    std::cout << "=========================================\n";

    std::cout << "Wallet      : " << m_Wallet << "\n";
    std::cout << "Pool        : " << m_SelectedPool << "\n";

    std::cout << "Hashrate    : " << m_Hashrate << " H/s\n";
    std::cout << "Temperature : " << m_Temperature << " C\n";
    std::cout << "Runtime     : " << m_Runtime << "\n\n";

    std::cout << "Submitted   : " << m_Submitted << "\n";
    std::cout << "Accepted    : " << m_Accepted << "\n";
    std::cout << "Rejected    : " << m_Rejected << "\n";

    std::cout << "Estimated Reward : "
              << m_EstimatedReward
              << " XMR\n";

    std::cout << "=========================================\n";
}

void Dashboard::Shutdown()
{
    std::cout << "Dashboard Closed.\n";
}

void Dashboard::SetWallet(const std::string& wallet)
{
    m_Wallet = wallet;
}

void Dashboard::SetPool(const std::string& pool)
{
    m_SelectedPool = pool;
}

void Dashboard::SetHashrate(double hashRate)
{
    m_Hashrate = hashRate;
}

void Dashboard::SetTemperature(double temperature)
{
    m_Temperature = temperature;
}

void Dashboard::SetRuntime(const std::string& runtime)
{
    m_Runtime = runtime;
}

void Dashboard::SetSubmitted(unsigned long long submitted)
{
    m_Submitted = submitted;
}

void Dashboard::SetAccepted(unsigned long long accepted)
{
    m_Accepted = accepted;
}

void Dashboard::SetRejected(unsigned long long rejected)
{
    m_Rejected = rejected;
}

void Dashboard::SetEstimatedReward(double reward)
{
    m_EstimatedReward = reward;
}
