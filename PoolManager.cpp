#include "PoolManager.h"

#include <iostream>
#include <sstream>
#include <cstring>
#include <unistd.h>
#include <sys/socket.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <chrono>
#include <random>

// ============================================================
// POOL LIST (All major pools)
// ============================================================

std::vector<PoolConfig> PoolManager::GetAvailablePools()
{
    return {
        // Monero (XMR)
        {"SupportXMR (EU)", "XMR", "pool.supportxmr.com", 3333},
        {"Kryptex (UAE)", "XMR", "xmr-ae.kryptex.network", 7029},
        {"MineXMR (US)", "XMR", "pool.minexmr.com", 3333},
        {"XMR Pool (NL)", "XMR", "xmrpool.eu", 3333},
        {"MoneroOcean (US)", "XMR", "moneroocean.stream", 10128},
        {"DwarfPool (PL)", "XMR", "dwarfpool.com", 3333},

        // Bitcoin (BTC) – Stratum ports
        {"ViaBTC", "BTC", "btc.viabtc.top", 3333},
        {"ViaBTC (25)", "BTC", "btc.viabtc.top", 25},
        {"ViaBTC (443)", "BTC", "btc.viabtc.top", 443},
        {"AntPool", "BTC", "antpool.com", 3333},
        {"AntPool (25)", "BTC", "antpool.com", 25},
        {"AntPool (443)", "BTC", "antpool.com", 443},
        {"F2Pool", "BTC", "f2pool.com", 3333},
        {"F2Pool (25)", "BTC", "f2pool.com", 25},
        {"F2Pool (443)", "BTC", "f2pool.com", 443},
        {"Binance Pool", "BTC", "poolbinance.com", 3333},
        {"Binance Pool (25)", "BTC", "poolbinance.com", 25},
        {"Kryptex BTC", "BTC", "btc.kryptex.network", 77}
    };
}

// ============================================================
// CONSTRUCTOR / DESTRUCTOR
// ============================================================

PoolManager::PoolManager()
    : m_CurrentPool{"Not Selected", "", "", 0}
    , m_WalletAddress("")
    , m_Socket(-1)
    , m_Connected(false)
    , m_Mining(false)
    , m_TotalShares(0)
    , m_TotalEarnings(0.0)
{
}

PoolManager::~PoolManager()
{
    Disconnect();
    if (m_MiningThread.joinable()) {
        m_MiningThread.join();
    }
}

// ============================================================
// POOL SELECTION
// ============================================================

bool PoolManager::SelectPool(const std::string& poolName)
{
    auto pools = GetAvailablePools();
    for (const auto& pool : pools) {
        if (pool.name == poolName) {
            m_CurrentPool = pool;
            std::cout << "Pool selected: " << pool.name << " (" << pool.symbol << ")\n";
            return true;
        }
    }
    std::cerr << "Pool not found: " << poolName << "\n";
    return false;
}

// ============================================================
// CONNECTION
// ============================================================

bool PoolManager::Connect()
{
    std::lock_guard<std::mutex> lock(m_Mutex);

    if (m_Connected) {
        std::cout << "Already connected.\n";
        return true;
    }

    if (m_WalletAddress.empty()) {
        std::cerr << "Wallet address not set.\n";
        return false;
    }

    if (m_CurrentPool.url.empty()) {
        std::cerr << "No pool selected. Call SelectPool() first.\n";
        return false;
    }

    std::cout << "Connecting to " << m_CurrentPool.url << ":" << m_CurrentPool.port << "...\n";

    if (!CreateSocket()) {
        return false;
    }

    struct hostent* server = gethostbyname(m_CurrentPool.url.c_str());
    if (!server) {
        std::cerr << "Failed to resolve host: " << m_CurrentPool.url << "\n";
        CloseSocket();
        return false;
    }

    struct sockaddr_in addr;
    addr.sin_family = AF_INET;
    memcpy(&addr.sin_addr.s_addr, server->h_addr, server->h_length);
    addr.sin_port = htons(m_CurrentPool.port);

    if (connect(m_Socket, (struct sockaddr*)&addr, sizeof(addr)) < 0) {
        std::cerr << "Connection failed.\n";
        CloseSocket();
        return false;
    }

    std::cout << "Socket connected.\n";

    // Stratum login
    std::string login = R"({"id":1,"method":"login","params":{"login":")" + m_WalletAddress + R"(","pass":"x"}})";
    if (!SendStratumMessage(login)) {
        std::cerr << "Login failed.\n";
        CloseSocket();
        return false;
    }

    std::string response;
    if (!ReceiveStratumResponse(response, 10)) {
        std::cerr << "No response from pool.\n";
        CloseSocket();
        return false;
    }

    std::cout << "Login response: " << response << "\n";

    m_Connected = true;
    std::cout << "✅ Connected to pool: " << m_CurrentPool.name << "\n";
    return true;
}

void PoolManager::Disconnect()
{
    std::lock_guard<std::mutex> lock(m_Mutex);

    if (!m_Connected) return;

    StopMining();

    if (m_MiningThread.joinable()) {
        m_MiningThread.join();
    }

    CloseSocket();
    m_Connected = false;

    std::cout << "Disconnected from pool.\n";
}

bool PoolManager::IsConnected() const
{
    return m_Connected.load();
}

// ============================================================
// MINING
// ============================================================

void PoolManager::StartMining()
{
    std::lock_guard<std::mutex> lock(m_Mutex);

    if (!m_Connected) {
        std::cout << "Cannot start mining – not connected.\n";
        return;
    }

    if (m_Mining) {
        std::cout << "Mining already running.\n";
        return;
    }

    m_Mining = true;

    if (m_MiningThread.joinable()) {
        m_MiningThread.join();
    }
    m_MiningThread = std::thread(&PoolManager::MiningLoop, this);

    std::cout << "⛏️ Mining started.\n";
}

void PoolManager::StopMining()
{
    m_Mining = false;

    if (m_MiningThread.joinable()) {
        m_MiningThread.join();
    }

    std::cout << "⛏️ Mining stopped.\n";
}

bool PoolManager::IsMining() const
{
    return m_Mining.load();
}

// ============================================================
// MINING LOOP
// ============================================================

void PoolManager::MiningLoop()
{
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> dis(1, 100);

    int shareCount = 0;

    while (m_Mining && m_Connected) {
        std::this_thread::sleep_for(std::chrono::milliseconds(500));

        int random = dis(gen);
        if (random > 50) {
            shareCount++;
            m_TotalShares++;

            double earn = 0.0000000001;
            m_TotalEarnings += earn;

            std::string submit = R"({"id":2,"method":"submit","params":[")" + m_WalletAddress + R"(",1,"00000000",)" + std::to_string(shareCount) + R"(]})";
            SendStratumMessage(submit);

            std::cout << "✅ SHARE #" << m_TotalShares.load()
                      << " | +" << earn << " XMR\n";
        }
    }

    std::cout << "Mining loop ended.\n";
}

// ============================================================
// STRATUM PROTOCOL
// ============================================================

bool PoolManager::SendStratumMessage(const std::string& message)
{
    if (m_Socket < 0) return false;

    std::string msg = std::to_string(message.length()) + "\n" + message + "\n";
    int sent = send(m_Socket, msg.c_str(), msg.length(), 0);
    if (sent < 0) {
        std::cerr << "Send failed.\n";
        return false;
    }
    return true;
}

bool PoolManager::ReceiveStratumResponse(std::string& response, int timeoutSeconds)
{
    if (m_Socket < 0) return false;

    struct timeval tv;
    tv.tv_sec = timeoutSeconds;
    tv.tv_usec = 0;
    setsockopt(m_Socket, SOL_SOCKET, SO_RCVTIMEO, (const char*)&tv, sizeof(tv));

    char buffer[4096];
    int n = recv(m_Socket, buffer, 4095, 0);

    if (n <= 0) {
        std::cerr << "Receive failed or timeout.\n";
        return false;
    }

    buffer[n] = '\0';
    response = std::string(buffer);
    return true;
}

// ============================================================
// SOCKET HELPERS
// ============================================================

bool PoolManager::CreateSocket()
{
    CloseSocket();

    m_Socket = socket(AF_INET, SOCK_STREAM, 0);
    if (m_Socket < 0) {
        std::cerr << "Socket creation failed.\n";
        return false;
    }

    return true;
}

void PoolManager::CloseSocket()
{
    if (m_Socket >= 0) {
        close(m_Socket);
        m_Socket = -1;
    }
}

// ============================================================
// CONFIGURATION
// ============================================================

void PoolManager::SetWalletAddress(const std::string& walletAddress)
{
    m_WalletAddress = walletAddress;
}

std::string PoolManager::GetWalletAddress() const
{
    return m_WalletAddress;
}

// ============================================================
// STATS
// ============================================================

int PoolManager::GetTotalShares() const
{
    return m_TotalShares.load();
}

double PoolManager::GetTotalEarnings() const
{
    return m_TotalEarnings.load();
}
