// ============================================================
// MARDUK WALLET — THE MINISTER (TRANSLATOR + STATUS SERVER)
// ============================================================
// Compile: g++ -std=c++11 -pthread -O3 -o wallet wallet.cpp -lssl -lcrypto
// Run: ./wallet
// Then open index.html in browser
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
// WEB SERVER – Serves /status JSON to Index
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
            cout << "🌐 Minister (Wallet) serving at http://127.0.0.1:8080/status\n";

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

int main() {
    cout << "\n════════════════════════════════════════════════════\n";
    cout << "👑 MINISTER (WALLET) — Translator & Status Server\n";
    cout << "════════════════════════════════════════════════════\n";
    cout << "📤 Treasury: " << WALLET << "\n";
    cout << "────────────────────────────────────────────────────\n";

    WebServer web;
    web.start();

    cout << "⏳ Minister waiting for Worker...\n";
    cout << "📊 King can check progress at http://127.0.0.1:8080/status\n";
    cout << "────────────────────────────────────────────────────\n";

    while (true) {
        this_thread::sleep_for(chrono::seconds(1));
    }

    web.stop();
    return 0;
}
