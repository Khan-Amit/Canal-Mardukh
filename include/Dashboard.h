#ifndef CANAL_MARDUKH_DASHBOARD_H
#define CANAL_MARDUKH_DASHBOARD_H

#include <string>

class Dashboard
{
public:

    Dashboard();

    ~Dashboard();

    void Initialize();

    void Draw();

    void Shutdown();

    void SetWallet(const std::string& wallet);

    void SetPool(const std::string& pool);

    void SetHashrate(double hashRate);

    void SetTemperature(double temperature);

    void SetRuntime(const std::string& runtime);

    void SetSubmitted(unsigned long long submitted);

    void SetAccepted(unsigned long long accepted);

    void SetRejected(unsigned long long rejected);

    void SetEstimatedReward(double reward);

private:

    std::string m_Wallet;
    std::string m_SelectedPool;

    double m_Hashrate;
    double m_Temperature;

    std::string m_Runtime;

    unsigned long long m_Submitted;
    unsigned long long m_Accepted;
    unsigned long long m_Rejected;

    double m_EstimatedReward;
};

#endif
