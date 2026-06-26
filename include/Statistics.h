#ifndef CANAL_MARDUKH_STATISTICS_H
#define CANAL_MARDUKH_STATISTICS_H

#include <string>

class Statistics
{
public:

    Statistics();

    ~Statistics();

    void Reset();

    void Update();

    void SetHashrate(double hashRate);
    void SetTemperature(double temperature);
    void SetRuntime(const std::string& runtime);

    void IncrementSubmitted();
    void IncrementAccepted();
    void IncrementRejected();

    void SetEstimatedReward(double reward);

    double GetHashrate() const;
    double GetTemperature() const;
    std::string GetRuntime() const;

    unsigned long long GetSubmitted() const;
    unsigned long long GetAccepted() const;
    unsigned long long GetRejected() const;

    double GetEstimatedReward() const;

private:

    double m_Hashrate;
    double m_Temperature;

    std::string m_Runtime;

    unsigned long long m_Submitted;
    unsigned long long m_Accepted;
    unsigned long long m_Rejected;

    double m_EstimatedReward;
};

#endif
