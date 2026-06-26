#include "Statistics.h"

Statistics::Statistics()
    : m_Hashrate(0.0),
      m_Temperature(0.0),
      m_Runtime("00:00:00"),
      m_Submitted(0),
      m_Accepted(0),
      m_Rejected(0),
      m_EstimatedReward(0.0)
{
}

Statistics::~Statistics()
{
}

void Statistics::Reset()
{
    m_Hashrate = 0.0;
    m_Temperature = 0.0;

    m_Runtime = "00:00:00";

    m_Submitted = 0;
    m_Accepted = 0;
    m_Rejected = 0;

    m_EstimatedReward = 0.0;
}

void Statistics::Update()
{
    // Future implementation:
    // Update hashrate, runtime,
    // temperature and reward estimates.
}

void Statistics::SetHashrate(double hashRate)
{
    m_Hashrate = hashRate;
}

void Statistics::SetTemperature(double temperature)
{
    m_Temperature = temperature;
}

void Statistics::SetRuntime(const std::string& runtime)
{
    m_Runtime = runtime;
}

void Statistics::IncrementSubmitted()
{
    ++m_Submitted;
}

void Statistics::IncrementAccepted()
{
    ++m_Accepted;
}

void Statistics::IncrementRejected()
{
    ++m_Rejected;
}

void Statistics::SetEstimatedReward(double reward)
{
    m_EstimatedReward = reward;
}

double Statistics::GetHashrate() const
{
    return m_Hashrate;
}

double Statistics::GetTemperature() const
{
    return m_Temperature;
}

std::string Statistics::GetRuntime() const
{
    return m_Runtime;
}

unsigned long long Statistics::GetSubmitted() const
{
    return m_Submitted;
}

unsigned long long Statistics::GetAccepted() const
{
    return m_Accepted;
}

unsigned long long Statistics::GetRejected() const
{
    return m_Rejected;
}

double Statistics::GetEstimatedReward() const
{
    return m_EstimatedReward;
}
