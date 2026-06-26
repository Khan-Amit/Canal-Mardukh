#include "PoolManager.h"

#include <iostream>

PoolManager::PoolManager()
    : m_PoolName("Not Selected"),
      m_PoolURL(""),
      m_WalletAddress(""),
      m_Connected(false),
      m_Mining(false)
{
}

PoolManager::~PoolManager()
{
}

bool PoolManager::Connect()
{
    std::cout << "Connecting to Pool...\n";

    m_Connected = true;

    std::cout << "Connection Successful.\n";

    return true;
}

void PoolManager::Disconnect()
{
    if (!m_Connected)
        return;

    std::cout << "Disconnecting...\n";

    m_Connected = false;
    m_Mining = false;

    std::cout << "Disconnected.\n";
}

bool PoolManager::IsConnected() const
{
    return m_Connected;
}

void PoolManager::StartMining()
{
    if (!m_Connected)
    {
        std::cout << "Cannot start. Pool is not connected.\n";
        return;
    }

    m_Mining = true;

    std::cout << "Mining Started.\n";
}

void PoolManager::StopMining()
{
    m_Mining = false;

    std::cout << "Mining Stopped.\n";
}

bool PoolManager::IsMining() const
{
    return m_Mining;
}

void PoolManager::SetPoolName(const std::string& poolName)
{
    m_PoolName = poolName;
}

void PoolManager::SetPoolURL(const std::string& poolURL)
{
    m_PoolURL = poolURL;
}

void PoolManager::SetWalletAddress(const std::string& walletAddress)
{
    m_WalletAddress = walletAddress;
}

std::string PoolManager::GetPoolName() const
{
    return m_PoolName;
}

std::string PoolManager::GetPoolURL() const
{
    return m_PoolURL;
}

std::string PoolManager::GetWalletAddress() const
{
    return m_WalletAddress;
}
