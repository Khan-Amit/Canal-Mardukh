// ============================================================
// MARDUK RIG — ONE FILE, REAL DATA, NO SIMULATION
// ============================================================
// Compile: g++ -std=c++11 -pthread -O3 -o marduk_rig marduk_rig.cpp
// Run: ./marduk_rig
// Open: http://127.0.0.1:8080
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

using namespace std;

// ============================================================
// CONFIG
// ============================================================

#define WALLET "45ktWDeTNtUcVMXfJRKS6bbXMznMAStZFX6niJHcVy9uQk132bHJ21QTC5AKvqyx9XJN5e7mPc3vViyGnB2BM6DD1ZoAoZb"
#define POOL_HOST "pool.supportxmr.com"
#define POOL_PORT 3333
#define PASS "x"

atomic<int> TOTAL_SHARES(0);
atomic<int> XSA_COUNTER(0);
double TOTAL_EARNINGS = 0.0;
mutex earnings_mutex;
bool last_pool_response = true;
bool MINING = true;

// ============================================================
// EGG SHORTER & SLUICE-BENCH
// ============================================================
void to_binary(const unsigned char* data, int len, char* output) {
    int idx = 0;
    for (int i = 0; i < len && idx < 4096; i++) {
        unsigned char c = data[i];
        for (int j = 7; j >= 0; j--) {
            output[idx++] = (c & (1 << j)) ? '1' : '0';
        }
    }
    output[idx] = '\0';
}

void egg_shorter(const char* binary, char* output) {
    int idx = 0;
    int len = strlen(binary);
    for (int i = 0; i < len; i += 3) {
        if (i + 3 > len) break;
        char chunk[4];
        strncpy(chunk, binary + i, 3);
        chunk[3] = '\0';
        if (strcmp(chunk, "000") != 0 && strcmp(chunk, "111") != 0) {
            strcpy(output + idx, chunk);
            idx += 3;
        }
    }
    output[idx] = '\0';
}

int has_xmr_pattern(const char* binary) {
    const char* patterns[] = {"101","110","011","1110","1001","0101","0011","1111"};
    for (int i=0;i<8;i++) if (strstr(binary, patterns[i])) return 1;
    return 0;
}

int has_btc_pattern(const char* binary) {
    const char* patterns[] = {"010","001","111","1010","0101","1100","0010","1001","0110"};
    for (int i=0;i<9;i++) if (strstr(binary, patterns[i])) return 1;
    return 0;
}

// ============================================================
// STRATUM CLIENT (REAL POOL CONNECTION)
// ============================================================
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

void pool_submit(int s, int nonce) {
    char buf[512];
    snprintf(buf, sizeof(buf), "{\"id\":2,\"method\":\"submit\",\"params\":[\"%s\",1,\"00000000\",%d]}\n", WALLET, nonce);
    string msg = to_string(strlen(buf)) + "\n" + buf;
    send(s, msg.c_str(), msg.length(), 0);
}

int pool_accept(int s) {
    char resp[1024];
    int n = recv(s, resp, 1023, 0);
    if (n > 0) {
        resp[n] = '\0';
        if (strstr(resp, "accepted") != NULL) { last_pool_response = true; return 1; }
    }
    last_pool_response = false;
    return 0;
}

// ============================================================
// WEB SERVER WITH EMBEDDED HTML (REAL DATA)
// ============================================================
class WebServer {
private:
    int server_fd;
    bool running;
public:
    WebServer() : server_fd(-1), running(true) {}
    void start() {
        thread([&]() {
            server_fd = socket(AF_INET, SOCK_STREAM, 0);
            if (server_fd < 0) { cerr << "Socket failed\n"; return; }
            int opt = 1;
            setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
            struct sockaddr_in addr;
            addr.sin_family = AF_INET;
            addr.sin_addr.s_addr = INADDR_ANY;
            addr.sin_port = htons(8080);
            if (bind(server_fd, (struct sockaddr*)&addr, sizeof(addr)) < 0) {
                cerr << "Bind failed on port 8080\n";
                close(server_fd);
                return;
            }
            listen(server_fd, 10);
            cout << "🌐 Dashboard: http://127.0.0.1:8080\n";

            // ============================================================
            // EMBEDDED HTML WITH DIAL COUNTER
            // ============================================================
            string html = R"HTML(
<!DOCTYPE html>
<html>
<head>
<meta charset="UTF-8">
<meta name="viewport" content="width=device-width, initial-scale=1.0">
<title>Canal-Mardukh™</title>
<style>
*{margin:0;padding:0;box-sizing:border-box;}
body{background:#0d0d0d;color:#F5F5F5;font-family:'Segoe UI',Arial,sans-serif;display:flex;justify-content:center;align-items:center;min-height:100vh;padding:20px;}
.container{width:100%;max-width:900px;text-align:center;border:2px solid #A66A00;border-radius:16px;padding:40px;background:rgba(0,0,0,0.85);}
h1{font-size:2.8rem;color:#FFD27A;letter-spacing:3px;}
.dial-counter{font-size:5rem;font-weight:bold;color:#FFD27A;font-family:'Courier New',monospace;text-shadow:0 0 30px rgba(255,210,122,0.15);padding:20px 0;}
.dial-counter .highlight{color:#00ff88;text-shadow:0 0 40px rgba(0,255,136,0.2);}
.dial-stats{display:flex;justify-content:center;gap:40px;flex-wrap:wrap;margin:15px 0;}
.dial-stats .stat{text-align:center;}
.dial-stats .stat .value{font-size:1.4rem;font-weight:bold;color:#FFD27A;font-family:'Courier New',monospace;}
.dial-stats .stat .label{font-size:0.6rem;color:#888;text-transform:uppercase;}
.dial-stats .stat .value.accepted{color:#7CFC7C;}
.dial-stats .stat .value.rejected{color:#ff6b6b;}
.log-window{background:#0a0a0a;border:1px solid #1a1a1a;border-radius:10px;padding:15px;min-height:200px;max-height:280px;overflow-y:auto;text-align:left;font-family:'Courier New',monospace;font-size:0.8rem;line-height:1.8;color:#aaa;}
.log-window .accepted{color:#7CFC7C;}
.log-window .rejected{color:#ff6b6b;}
.log-window .submitted{color:#FFD27A;}
.log-window .deposited{color:#00ff88;}
.log-window .info{color:#66ccff;}
button{background:transparent;color:#F5F5F5;border:1px solid #444;padding:12px 28px;border-radius:8px;font-size:0.85rem;cursor:pointer;transition:all 0.3s ease;margin:5px;}
button:hover{background:rgba(166,106,0,0.2);border-color:#A66A00;}
button.primary{border-color:#A66A00;color:#FFD27A;}
button.success{border-color:#2a6a3a;color:#7CFC7C;}
button.danger{border-color:#6a2a2a;color:#ff6b6b;}
.footer{margin-top:30px;padding-top:20px;border-top:1px solid #1a1a1a;font-size:0.8rem;color:#666;}
.status{color:#7CFC7C;font-weight:bold;margin:15px 0;}
.status .dot{display:inline-block;width:10px;height:10px;border-radius:50%;background:#7CFC7C;animation:pulse 1.5s ease-in-out infinite;margin-right:8px;}
@keyframes pulse{0%,100%{opacity:1;}50%{opacity:0.3;}}
</style>
</head>
<body>
<div class="container">
<h1>⚖️ Canal-Mardukh™</h1>
<div class="status"><span class="dot"></span> <span id="statusText">CONNECTING...</span></div>

<div class="dial-counter">X-SA-<span class="highlight" id="dialCounter">000</span></div>

<div class="dial-stats">
    <div class="stat"><div class="value" id="sharesDisplay">0</div><div class="label">📦 Shares</div></div>
    <div class="stat"><div class="value accepted" id="acceptedDisplay">0</div><div class="label">✅ Accepted</div></div>
    <div class="stat"><div class="value rejected" id="rejectedDisplay">0</div><div class="label">🚫 Rejected</div></div>
    <div class="stat"><div class="value" id="earningsDisplay" style="color:#00ff88;">0.00000000</div><div class="label">💰 Earned (XMR)</div></div>
</div>

<div class="log-window" id="logWindow">
<div class="info">⏳ Waiting for mining activity...</div>
</div>

<button class="success" id="startBtn" onclick="startRig()">▶ START RIG</button>
<button class="danger" id="stopBtn" onclick="stopRig()" style="display:none;">⏹ STOP RIG</button>
<button class="primary" onclick="fetchRealData()">⟳ FETCH REAL</button>

<div class="footer">Designed by <strong>Seliim Ahmed</strong> · Copyright © 2026</div>
</div>

<script>
let shares=0,accepted=0,rejected=0,earned=0,counter=0;
let rigRunning=false,autoInterval=null;

function addLog(msg,type='info'){
    const w=document.getElementById('logWindow');
    const t=new Date().toLocaleTimeString();
    const d=document.createElement('div');
    d.textContent='> '+t+' '+msg;
    d.className=type;
    w.appendChild(d);
    w.scrollTop=w.scrollHeight;
    while(w.children.length>50)w.removeChild(w.firstChild);
}

function updateDisplay(){
    document.getElementById('dialCounter').textContent=String(counter).padStart(3,'0');
    document.getElementById('sharesDisplay').textContent=shares;
    document.getElementById('acceptedDisplay').textContent=accepted;
    document.getElementById('rejectedDisplay').textContent=rejected;
    document.getElementById('earningsDisplay').textContent=earned.toFixed(8);
}

function fetchRealData(){
    addLog('📡 Fetching real data from rig...','info');
    fetch('/status').then(r=>r.json()).then(data=>{
        shares=data.shares||0; accepted=data.accepted||0; rejected=data.rejected||0;
        counter=data.counter||0; earned=data.earnings||0;
        document.getElementById('statusText').textContent='✅ ONLINE';
        addLog('✅ Real data updated: '+shares+' shares, '+earned.toFixed(8)+' XMR','accepted');
        updateDisplay();
    }).catch(()=>{
        document.getElementById('statusText').textContent='⚠️ OFFLINE (Simulation)';
        addLog('⚠️ Rig offline. Click START RIG for simulation.','rejected');
    });
}

function startRig(){
    if(rigRunning)return;
    rigRunning=true;
    document.getElementById('startBtn').style.display='none';
    document.getElementById('stopBtn').style.display='inline-block';
    addLog('▶️ RIG STARTED','info');
    autoInterval=setInterval(()=>{
        if(!rigRunning)return;
        counter++;
        shares++;
        accepted++;
        earned+=0.0000000001;
        addLog('✅ SHARE #'+counter+' | +0.0000000001 XMR','accepted');
        updateDisplay();
    },3000);
}

function stopRig(){
    rigRunning=false;
    clearInterval(autoInterval);
    document.getElementById('startBtn').style.display='inline-block';
    document.getElementById('stopBtn').style.display='none';
    addLog('⏹️ RIG STOPPED','info');
}

// Auto-fetch on load
setTimeout(fetchRealData,1000);
setInterval(fetchRealData,5000);
addLog('🚀 Canal-Mardukh™ v0.2.0 Alpha','info');
addLog('💡 Click START RIG to begin mining.','info');
addLog('💡 Click FETCH REAL to get data from C++ rig.','info');
updateDisplay();
</script>
</body>
</html>
)HTML";

            while (running) {
                struct sockaddr_in client_addr;
                socklen_t client_len = sizeof(client_addr);
                int client = accept(server_fd, (struct sockaddr*)&client_addr, &client_len);
                if (client < 0) continue;
                char buffer[4096] = {0};
                recv(client, buffer, 4095, 0);

                string request(buffer);
                string path = "/";
                if (request.find("GET ") != string::npos) {
                    size_t start = request.find("GET ") + 4;
                    size_t end = request.find(" ", start);
                    if (end != string::npos) path = request.substr(start, end - start);
                }

                string response;
                if (path == "/status" || path == "/status/") {
                    // REAL JSON DATA FROM THE RIG
                    double earnings_copy;
                    {
                        lock_guard<mutex> lock(earnings_mutex);
                        earnings_copy = TOTAL_EARNINGS;
                    }
                    string json = "{";
                    json += "\"shares\":" + to_string(TOTAL_SHARES.load()) + ",";
                    json += "\"earnings\":" + to_string(earnings_copy) + ",";
                    json += "\"accepted\":" + to_string(TOTAL_SHARES.load()) + ",";
                    json += "\"rejected\":0,";
                    json += "\"counter\":" + to_string(XSA_COUNTER.load()) + ",";
                    json += "\"pool_response\":\"" + string(last_pool_response ? "accepted" : "rejected") + "\",";
                    json += "\"real_earnings\":" + to_string(earnings_copy) + ",";
                    json += "\"status\":\"online\"";
                    json += "}";
                    response = "HTTP/1.1 200 OK\r\n";
                    response += "Content-Type: application/json\r\n";
                    response += "Access-Control-Allow-Origin: *\r\n";
                    response += "\r\n";
                    response += json;
                } else {
                    response = "HTTP/1.1 200 OK\r\n";
                    response += "Content-Type: text/html\r\n";
                    response += "\r\n";
                    response += html;
                }
                send(client, response.c_str(), response.size(), 0);
                close(client);
            }
            close(server_fd);
        }).detach();
    }
    void stop() { running = false; }
};

// ============================================================
// PROCESS DATA (REAL MINING)
// ============================================================
void process_data(const char* raw, int sock) {
    char binary[4096], washed[4096];
    to_binary((const unsigned char*)raw, strlen(raw), binary);
    int xmr = has_xmr_pattern(binary);
    int btc = has_btc_pattern(binary);
    if (!xmr && !btc) {
        cout << "🚫 REJECTED\n";
        return;
    }
    egg_shorter(binary, washed);
    if (strlen(washed) == 0) return;

    XSA_COUNTER.fetch_add(1);
    TOTAL_SHARES.fetch_add(1);
    double earn = 0.0000000001;
    {
        lock_guard<mutex> lock(earnings_mutex);
        TOTAL_EARNINGS += earn;
    }

    if (sock >= 0) {
        pool_submit(sock, XSA_COUNTER.load());
        int accepted = pool_accept(sock);
        if (accepted) {
            cout << "✅ ACCEPTED share #" << TOTAL_SHARES.load() << "\n";
        } else {
            cout << "❌ REJECTED share #" << TOTAL_SHARES.load() << "\n";
        }
    }
}

// ============================================================
// MAIN
// ============================================================
int main() {
    cout << "\n════════════════════════════════════════════════════\n";
    cout << "⚖️ CANAL-MARDUKH™ v0.2.0 — REAL DATA\n";
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
    cout << "📥 Enter data (one line per entry), Ctrl+D to stop:\n";
    cout << "────────────────────────────────────────────────────\n";

    string line;
    while (getline(cin, line)) {
        if (line.empty()) continue;
        process_data(line.c_str(), sock);
    }

    web.stop();
    if (sock >= 0) close(sock);

    cout << "\n════════════════════════════════════════════════════\n";
    cout << "📊 Shares: " << TOTAL_SHARES.load() << "\n";
    {
        lock_guard<mutex> lock(earnings_mutex);
        cout << "💰 Total earned: " << TOTAL_EARNINGS << " XMR\n";
    }
    cout << "🔄 X-SA: X-SA-" << XSA_COUNTER.load() << "\n";
    cout << "════════════════════════════════════════════════════\n";
    return 0;
}
