#ifndef CANAL_MARDUKH_CONFIGURATION_H
#define CANAL_MARDUKH_CONFIGURATION_H

#include <string>

class Configuration
{
public:

    Configuration();

    ~Configuration();

    bool Load();

    bool Save();

    void SetWalletAddress(const std::string& wallet);

    void SetPoolName(const std::string& pool);

    void SetPoolURL(const std::string& url);

    void SetThreadCount(unsigned int threads);

    void SetTheme(const std::string& theme);

    std::string GetWalletAddress() const;

    std::string GetPoolName() const;

    std::string GetPoolURL() const;

    unsigned int GetThreadCount() const;

    std::string GetTheme() const;

private:

    std::string m_WalletAddress;
    std::string m_PoolName;
    std::string m_PoolURL;

    unsigned int m_ThreadCount;

    std::string m_Theme;
};

#endif
