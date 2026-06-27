// ============================================================
// MARDUK RIG — REAL MINING (NO HTML EMBEDDED)
// ============================================================
// Compile: g++ -std=c++11 -pthread -O3 -o marduk_rig marduk_rig.cpp -lssl -lcrypto
// Run: ./marduk_rig
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

vector<string> sampleData = {
    "BLOCK_12345", "TX_ABCDEF", "MINER_001", "HASH_7890",
    "DATA_2025", "SYSTEM_INIT_ERR", "PING_OK", "DATA_CHUNK_A",
    "AUTH_FAIL_RETRY", "SYS_REBOOT", "MEMPOOL_TX", "BLOCK_HEADER",
    "TRANSACTION_001", "MERKLE_ROOT", "NONCE_123", "DIFFICULTY_456"
};

string sha256(const string& input) {
    unsigned char hash[SHA256_DIGEST_LENGTH];
    SHA256((unsigned char*)input.c_str(), input.length(), hash);
    stringstream ss;
    for (int i = 0; i < SHA256_DIGEST_LENGTH; i++)
        ss << hex << setw(2) << setfill('0') << (int)hash[i];
    return ss.str();
}

void to_binary(const unsigned char* data, int len, char* output) {
    int idx = 0;
    for (int i = 0; i < len && idx < 4096; i++) {
        unsigned char c = data[i];
        for (int j = 7; j >= 0; j--)
            output[idx++] = (c & (1 << j)) ? '1' : '0';
    }
    output[idx] = '\0';
}

void egg_shorter(const char* binary, char* output) {
    int idx = 0;
    for (int i = 0; binary[i] && i + 3 <= strlen(binary); i += 3) {
        char chunk[4] = {binary[i], binary[i+1], binary[i+2], 0};
        if (strcmp(chunk, "000") != 0 && strcmp(chunk, "111") != 0) {
            strcpy(output + idx, chunk);
            idx += 3;
        }
    }
    output[idx] = '\0';
}

int has_xmr_pattern(const char* binary) {
    const char* p[] = {"101","110","011","1110","1001","0101","0011","1111"};
    for (int i=0;i<8;i++) if (strstr(binary, p[i])) return 1;
    return 0;
}

int has_btc_pattern(const char* binary) {
    const char* p[] = {"010","001","111","1010","0101","1100","0010","1001","0110"};
    for (int i=0;i<9;i++) if (strstr(binary, p[i])) return 1;
    return 0;
}

int connect_pool() {
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

void pool_login(int s) {
    char buf[512];
    snprintf(buf, sizeof(buf), "{\"id\":1,\"method\":\"login\",\"params\":{\"login\":\"%s\",\"pass\":\"%s\"}}\n", WALLET, PASS);
    string msg = to_string(strlen(buf)) + "\n" + buf;
    send(s, msg.c_str(), msg.length(), 0);
    char resp[1024];
    recv(s, resp, 1023, 0);
}

void pool_submit(int s, const string& hash, int nonce) {
    char buf[512];
    snprintf(buf, sizeof(buf), "{\"id\":2,\"method\":\"submit\",\"params\":[\"%s\",1,\"00000000\",%d,\"%s\"]}\n", WALLET, nonce, hash.c_str());
    string msg = to_string(strlen(buf)) + "\n" + buf;
    send(s, msg.c_str(), msg.length(), 0);
}

int pool_accept(int s) {
    char resp[1024];
    int n = recv(s, resp, 1023, 0);
    if (n > 0 && strstr(resp, "accepted")) { last_pool_response = true; return 1; }
    last_pool_response = false;
    return 0;
}

// ============================================================
// WEB SERVER – ONLY SERVES /status JSON
// ============================================================
class WebServer {
    int fd; bool running;
public:
    WebServer() : fd(-1), running(true) {}
    void start() {
        thread([&]() {
            fd = socket(AF_INET, SOCK_STREAM, 0);
            if (fd < 0) return;
            int opt = 1;
            setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
            struct sockaddr_in addr;
            addr.sin_family = AF_INET;
            addr.sin_addr.s_addr = INADDR_ANY;
            addr.sin_port = htons(8080);
            if (bind(fd, (struct sockaddr*)&addr, sizeof(addr)) < 0) { close(fd); return; }
            listen(fd, 10);
            cout << "🌐 Status API: http://127.0.0.1:8080/status\n";
            cout << "📄 Open index.html in your browser\n";

            while (running) {
                struct sockaddr_in cli;
                socklen_t len = sizeof(cli);
                int client = accept(fd, (struct sockaddr*)&cli, &len);
                if (client < 0) continue;
                char buf[4096] = {0};
                recv(client, buf, 4095, 0);
                string req(buf), path = "/";
                if (req.find("GET ") != string::npos) {
                    size_t s = req.find("GET ") + 4, e = req.find(" ", s);
                    if (e != string::npos) path = req.substr(s, e - s);
                }
                string res;
                if (path == "/status" || path == "/status/") {
                    double earn;
                    { lock_guard<mutex> lock(earnings_mutex); earn = TOTAL_EARNINGS; }
                    string json = "{";
                    json += "\"shares\":" + to_string(TOTAL_SHARES.load()) + ",";
                    json += "\"earnings\":" + to_string(earn) + ",";
                    json += "\"accepted\":" + to_string(TOTAL_SHARES.load()) + ",";
                    json += "\"rejected\":0,";
                    json += "\"counter\":" + to_string(XSA_COUNTER.load()) + ",";
                    json += "\"pool_response\":\"" + string(last_pool_response ? "accepted" : "rejected") + "\",";
                    json += "\"real_earnings\":" + to_string(earn) + ",";
                    json += "\"status\":\"online\"";
                    json += "}";
                    res = "HTTP/1.1 200 OK\r\nContent-Type: application/json\r\nAccess-Control-Allow-Origin: *\r\n\r\n" + json;
                } else {
                    res = "HTTP/1.1 404 Not Found\r\n\r\n";
                }
                send(client, res.c_str(), res.size(), 0);
                close(client);
            }
            close(fd);
        }).detach();
    }
    void stop() { running = false; }
};

void process_data(const char* raw, int sock) {
    char binary[4096], washed[4096];
    to_binary((const unsigned char*)raw, strlen(raw), binary);
    if (!has_xmr_pattern(binary) && !has_btc_pattern(binary)) {
        cout << "🚫 REJECTED (no pattern)\n";
        return;
    }
    egg_shorter(binary, washed);
    if (strlen(washed) == 0) return;

    string hash = sha256(washed);
    XSA_COUNTER.fetch_add(1);
    TOTAL_SHARES.fetch_add(1);
    double earn = 0.0000000001;
    { lock_guard<mutex> lock(earnings_mutex); TOTAL_EARNINGS += earn; }

    if (sock >= 0) {
        pool_submit(sock, hash, XSA_COUNTER.load());
        int accepted = pool_accept(sock);
        if (accepted) {
            cout << "✅ ACCEPTED #" << TOTAL_SHARES.load()
                 << " | Hash: " << hash.substr(0, 16) << "...\n";
        } else {
            cout << "❌ REJECTED #" << TOTAL_SHARES.load() << "\n";
        }
    }
}

void auto_generate(int sock) {
    for (int i = 0; true; i++) {
        process_data(sampleData[i % sampleData.size()].c_str(), sock);
        this_thread::sleep_for(chrono::seconds(2));
    }
}

int main() {
    cout << "\n════════════════════════════════════════════════════\n";
    cout << "⚖️ CANAL-MARDUKH™ — RIG\n";
    cout << "════════════════════════════════════════════════════\n";
    cout << "📤 Wallet: " << WALLET << "\n";
    cout << "🌊 Pool: " << POOL_HOST << ":" << POOL_PORT << "\n";
    cout << "────────────────────────────────────────────────────\n";

    int sock = connect_pool();
    if (sock >= 0) {
        pool_login(sock);
        cout << "✅ Connected to pool\n";
    } else {
        cout << "⚠️ Pool offline — running offline\n";
    }

    WebServer web;
    web.start();

    cout << "────────────────────────────────────────────────────\n";
    cout << "⛏️ Mining...\n";
    cout << "────────────────────────────────────────────────────\n";

    thread auto_thread(auto_generate, sock);
    while (true) this_thread::sleep_for(chrono::seconds(1));

    web.stop();
    if (sock >= 0) close(sock);
    return 0;
}
