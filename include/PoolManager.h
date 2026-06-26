#ifndef CANAL_MARDUKH_POOLMANAGER_H
#define CANAL_MARDUKH_POOLMANAGER_H

#include <string>

class PoolManager
{
public:

    PoolManager();

    ~PoolManager();

    bool Connect();

    void Disconnect();

    bool IsConnected() const;

    void StartMining();

    void StopMining();

    bool IsMining() const;

    void SetPoolName(const std::string& poolName);

    void SetPoolURL(const std::string& poolURL);

    void SetWalletAddress(const std::string& walletAddress);

    std::string GetPoolName() const;

    std::string GetPoolURL() const;

    std::string GetWalletAddress() const;

private:

    std::string m_PoolName;
    std::string m_PoolURL;
    std::string m_WalletAddress;

    bool m_Connected;
    bool m_Mining;
};

#endif
