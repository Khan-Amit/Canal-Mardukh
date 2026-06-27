#ifndef POOLMANAGER_H
#define POOLMANAGER_H

#include <string>
#include <atomic>
#include <thread>
#include <mutex>

class PoolManager {
public:
    PoolManager();
    ~PoolManager();

    // Connection
    bool Connect();
    void Disconnect();
    bool IsConnected() const;

    // Mining
    void StartMining();
    void StopMining();
    bool IsMining() const;

    // Configuration
    void SetPoolName(const std::string& poolName);
    void SetPoolURL(const std::string& poolURL);
    void SetPoolPort(int port);
    void SetWalletAddress(const std::string& walletAddress);

    std::string GetPoolName() const;
    std::string GetPoolURL() const;
    int GetPoolPort() const;
    std::string GetWalletAddress() const;

    // Stats
    int GetTotalShares() const;
    double GetTotalEarnings() const;

private:
    // Mining loop (runs in a thread)
    void MiningLoop();

    // Stratum protocol
    bool SendStratumMessage(const std::string& message);
    bool ReceiveStratumResponse(std::string& response, int timeoutSeconds = 5);

    // Socket
    bool CreateSocket();
    void CloseSocket();

    // State
    std::string m_PoolName;
    std::string m_PoolURL;
    int m_PoolPort;
    std::string m_WalletAddress;

    int m_Socket;
    std::atomic<bool> m_Connected;
    std::atomic<bool> m_Mining;
    std::atomic<int> m_TotalShares;
    std::atomic<double> m_TotalEarnings;

    std::thread m_MiningThread;
    mutable std::mutex m_Mutex;
};

#endif // POOLMANAGER_H
