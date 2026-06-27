// ============================================================
// MARDUK RIG — THE WORKER (COLLECTS, FILTERS, HASHES)
// ============================================================
// Compile: g++ -std=c++11 -pthread -O3 -o rig rig.cpp -lssl -lcrypto
// Run: ./rig
// ============================================================

#include <iostream>
#include <string>
#include <vector>
#include <algorithm>
#include <chrono>
#include <thread>
#include <fstream>
#include <sstream>
#include <cstring>
#include <netdb.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <atomic>
#include <mutex>
#include <cctype>
#include <random>
#include <iomanip>
#include <openssl/sha.h>

using namespace std;

#define WALLET "45ktWDeTNtUcVMXfJRKS6bbXMznMAStZFX6niJHcVy9uQk132bHJ21QTC5AKvqyx9XJN5e7mPc3vViyGnB2BM6DD1ZoAoZb"
#define POOL_HOST "pool.supportxmr.com"
#define POOL_PORT 3333
#define PASS "x"

atomic<int> TOTAL_SHARES(0);
atomic<int> XSA_COUNTER(0);
double TOTAL_EARNINGS = 0.0;
mutex earnings_mutex;
bool last_pool_response = true;

// ============================================================
// SLUICE-BENCH REFERENCE DATABASE (HS256 Patterns)
// ============================================================

const char* xmr_patterns[] = {"101","110","011","1110","1001","0101","0011","1111"};
const char* btc_patterns[] = {"010","001","111","1010","0101","1100","0010","1001","0110"};

string sha256(const string& input) {
    unsigned char hash[SHA256_DIGEST_LENGTH];
    SHA256((unsigned char*)input.c_str(), input.length(), hash);
    stringstream ss;
    for (int i = 0; i < SHA256_DIGEST_LENGTH; i++)
        ss << hex << setw(2) << setfill('0') << (int)hash[i];
    return ss.str();
}

string toBinary(const string& input) {
    string binary;
    for (unsigned char c : input) {
        for (int j = 7; j >= 0; j--)
            binary += (c & (1 << j)) ? '1' : '0';
    }
    return binary;
}

string eggShorter(const string& binary) {
    string result;
    for (int i = 0; i < (int)binary.length() && i + 3 <= (int)binary.length(); i += 3) {
        string chunk = binary.substr(i, 3);
        if (chunk != "000" && chunk != "111")
            result += chunk;
    }
    return result;
}

bool hasXMRPattern(const string& binary) {
    for (int i = 0; i < 8; i++) {
        if (binary.find(xmr_patterns[i]) != string::npos) return true;
    }
    return false;
}

bool hasBTCPattern(const string& binary) {
    for (int i = 0; i < 9; i++) {
        if (binary.find(btc_patterns[i]) != string::npos) return true;
    }
    return false;
}

int connectPool() {
    int s = socket(AF_INET, SOCK_STREAM, 0);
    if (s < 0) return -1;
    struct hostent* server = gethostbyname(POOL_HOST);
    if (!server) { close(s); return -1; }
    struct sockaddr_in addr;
    addr.sin_family = AF_INET;
    memcpy(&addr.sin_addr.s_addr, server->h_addr, server->h_length);
    addr.sin_port = htons(POOL_PORT);
    if (connect(s, (struct sockaddr*)&addr, sizeof(addr)) < 0) { close(s); return -1; }
    return s;
}

void poolLogin(int s) {
    char buf[512];
    snprintf(buf, sizeof(buf), "{\"id\":1,\"method\":\"login\",\"params\":{\"login\":\"%s\",\"pass\":\"%s\"}}\n", WALLET, PASS);
    string msg = to_string(strlen(buf)) + "\n" + buf;
    send(s, msg.c_str(), msg.length(), 0);
    char resp[1024];
    recv(s, resp, 1023, 0);
}

void poolSubmit(int s, const string& hash, int nonce) {
    char buf[512];
    snprintf(buf, sizeof(buf), "{\"id\":2,\"method\":\"submit\",\"params\":[\"%s\",1,\"00000000\",%d,\"%s\"]}\n", WALLET, nonce, hash.c_str());
    string msg = to_string(strlen(buf)) + "\n" + buf;
    send(s, msg.c_str(), msg.length(), 0);
}

bool poolAccept(int s) {
    char resp[1024];
    int n = recv(s, resp, 1023, 0);
    if (n > 0 && strstr(resp, "accepted")) { last_pool_response = true; return true; }
    last_pool_response = false;
    return false;
}

bool walletSendToPool(const string& hash, int nonce, int sock) {
    if (sock < 0) return false;
    poolSubmit(sock, hash, nonce);
    return poolAccept(sock);
}

string processData(const string& raw, int sock) {
    string binary = toBinary(raw);
    if (!hasXMRPattern(binary) && !hasBTCPattern(binary)) return "";
    string washed = eggShorter(binary);
    if (washed.empty()) return "";
    string hash = sha256(washed);
    XSA_COUNTER.fetch_add(1);
    TOTAL_SHARES.fetch_add(1);
    bool approved = walletSendToPool(hash, XSA_COUNTER.load(), sock);
    if (approved) {
        double earn = 0.0000000001;
        { lock_guard<mutex> lock(earnings_mutex); TOTAL_EARNINGS += earn; }
    }
    return hash;
}

vector<string> sampleData = {
    "BLOCK_12345", "TX_ABCDEF", "MINER_001", "HASH_7890",
    "DATA_2025", "SYSTEM_INIT_ERR", "PING_OK", "DATA_CHUNK_A",
    "AUTH_FAIL_RETRY", "SYS_REBOOT", "MEMPOOL_TX", "BLOCK_HEADER",
    "TRANSACTION_001", "MERKLE_ROOT", "NONCE_123", "DIFFICULTY_456"
};

void autoGenerate(int sock) {
    int idx = 0;
    while (true) {
        processData(sampleData[idx % sampleData.size()], sock);
        idx++;
        this_thread::sleep_for(chrono::seconds(3));
    }
}

int main() {
    cout << "\n════════════════════════════════════════════════════\n";
    cout << "⚙️ WORKER (RIG) — Collecting data from canal\n";
    cout << "════════════════════════════════════════════════════\n";
    cout << "📤 Wallet: " << WALLET << "\n";
    cout << "🌊 Pool: " << POOL_HOST << ":" << POOL_PORT << "\n";
    cout << "────────────────────────────────────────────────────\n";

    int sock = connectPool();
    if (sock >= 0) {
        poolLogin(sock);
        cout << "✅ Worker connected to pool\n";
    } else {
        cout << "⚠️ Worker offline — no pool connection\n";
    }

    cout << "────────────────────────────────────────────────────\n";
    cout << "⛏️ Worker: Mining...\n";
    cout << "────────────────────────────────────────────────────\n";

    thread auto_thread(autoGenerate, sock);
    while (true) this_thread::sleep_for(chrono::seconds(1));

    if (sock >= 0) close(sock);
    return 0;
}
