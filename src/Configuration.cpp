#include "Configuration.h"

#include <fstream>

Configuration::Configuration()
    : m_WalletAddress(""),
      m_PoolName(""),
      m_PoolURL(""),
      m_ThreadCount(1),
      m_Theme("Maroon")
{
}

Configuration::~Configuration()
{
}

bool Configuration::Load()
{
    std::ifstream file("config.cfg");

    if (!file.is_open())
    {
        return false;
    }

    std::getline(file, m_WalletAddress);
    std::getline(file, m_PoolName);
    std::getline(file, m_PoolURL);

    file >> m_ThreadCount;

    file.ignore();

    std::getline(file, m_Theme);

    file.close();

    return true;
}

bool Configuration::Save()
{
    std::ofstream file("config.cfg");

    if (!file.is_open())
    {
        return false;
    }

    file << m_WalletAddress << '\n';
    file << m_PoolName << '\n';
    file << m_PoolURL << '\n';
    file << m_ThreadCount << '\n';
    file << m_Theme << '\n';

    file.close();

    return true;
}

void Configuration::SetWalletAddress(const std::string& wallet)
{
    m_WalletAddress = wallet;
}

void Configuration::SetPoolName(const std::string& pool)
{
    m_PoolName = pool;
}

void Configuration::SetPoolURL(const std::string& url)
{
    m_PoolURL = url;
}

void Configuration::SetThreadCount(unsigned int threads)
{
    m_ThreadCount = threads;
}

void Configuration::SetTheme(const std::string& theme)
{
    m_Theme = theme;
}

std::string Configuration::GetWalletAddress() const
{
    return m_WalletAddress;
}

std::string Configuration::GetPoolName() const
{
    return m_PoolName;
}

std::string Configuration::GetPoolURL() const
{
    return m_PoolURL;
}

unsigned int Configuration::GetThreadCount() const
{
    return m_ThreadCount;
}

std::string Configuration::GetTheme() const
{
    return m_Theme;
}
