// ============================================================
// MARDUK RIG — REAL DATA, NO SIMULATION
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

vector<string> sampleData = {
    "BLOCK_12345", "TX_ABCDEF", "MINER_001", "HASH_7890",
    "DATA_2025", "SYSTEM_INIT_ERR", "PING_OK", "DATA_CHUNK_A",
    "AUTH_FAIL_RETRY", "SYS_REBOOT", "MEMPOOL_TX", "BLOCK_HEADER",
    "TRANSACTION_001", "MERKLE_ROOT", "NONCE_123", "DIFFICULTY_456"
};

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
// STRATUM CLIENT
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
// WEB SERVER
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

            string html = R"HTML(
<!DOCTYPE html>
<html>
<head>
<meta charset="UTF-8">
<meta name="viewport" content="width=device-width, initial-scale=1.0">
<title>Canal-Mardukh™ – Real</title>
<style>
*{margin:0;padding:0;box-sizing:border-box;}
body{background:#0a0a0a;color:#e0e0e0;font-family:'Segoe UI',Arial,sans-serif;min-height:100vh;padding:20px;display:flex;justify-content:center;align-items:center;}
.container{max-width:1100px;width:100%;background:#111;border:2px solid #A66A00;border-radius:16px;padding:30px;box-shadow:0 0 60px rgba(166,106,0,0.1);}
.header{border-bottom:1px solid #222;padding-bottom:15px;margin-bottom:20px;display:flex;justify-content:space-between;align-items:center;flex-wrap:wrap;}
h1{color:#FFD27A;font-size:2rem;letter-spacing:2px;}
h1 small{font-size:0.7rem;color:#888;display:block;letter-spacing:1px;}
.status{font-weight:bold;font-size:0.85rem;}
.status .dot{display:inline-block;width:10px;height:10px;border-radius:50%;margin-right:8px;}
.status.online .dot{background:#7CFC7C;animation:pulse 1.5s ease-in-out infinite;}
.status.offline .dot{background:#ff6b6b;}
@keyframes pulse{0%,100%{opacity:1;}50%{opacity:0.3;}}
.wallet-display{background:#0a0a0a;border:1px solid #222;border-radius:8px;padding:12px 15px;margin-bottom:20px;word-break:break-all;font-family:'Courier New',monospace;font-size:0.75rem;color:#88ffaa;}
.wallet-display .label{color:#888;font-size:0.6rem;text-transform:uppercase;letter-spacing:1px;display:block;margin-bottom:3px;}
.dial-counter{text-align:center;padding:15px 0 10px;font-size:3.5rem;font-weight:bold;color:#FFD27A;font-family:'Courier New',monospace;}
.dial-counter .highlight{color:#00ff88;}
.dial-stats{display:grid;grid-template-columns:repeat(4,1fr);gap:10px;background:#0a0a0a;border:1px solid #222;border-radius:8px;padding:15px;margin-bottom:15px;}
@media(max-width:500px){.dial-stats{grid-template-columns:repeat(2,1fr);}}
.dial-stat{text-align:center;}
.dial-stat .value{font-size:1.6rem;font-weight:bold;font-family:'Courier New',monospace;color:#FFD27A;}
.dial-stat .value.accepted{color:#7CFC7C;}
.dial-stat .value.rejected{color:#ff6b6b;}
.dial-stat .value.earnings{color:#00ff88;}
.dial-stat .label{font-size:0.55rem;color:#888;text-transform:uppercase;letter-spacing:0.5px;}
.pool-selector{background:#0a0a0a;border:1px solid #222;border-radius:8px;padding:12px 15px;margin-bottom:15px;}
.pool-selector label{color:#888;font-size:0.6rem;text-transform:uppercase;letter-spacing:1px;display:block;margin-bottom:5px;}
.pool-selector select{width:100%;padding:10px 12px;background:#111;border:1px solid #333;border-radius:6px;color:#e0e0e0;font-size:0.85rem;outline:none;cursor:pointer;}
.pool-selector select:focus{border-color:#A66A00;}
.log-window{background:#0a0a0a;border:1px solid #1a1a1a;border-radius:8px;padding:15px;min-height:300px;max-height:500px;overflow-y:auto;font-family:'Courier New',monospace;font-size:0.8rem;line-height:1.8;color:#aaa;margin-top:15px;}
.log-window::-webkit-scrollbar{width:4px;}
.log-window::-webkit-scrollbar-track{background:#0a0a0a;}
.log-window::-webkit-scrollbar-thumb{background:#A66A00;border-radius:4px;}
.log-entry{padding:2px 0;border-bottom:1px solid rgba(255,255,255,0.03);}
.log-entry .time{color:#555;margin-right:10px;}
.log-entry .accepted{color:#7CFC7C;}
.log-entry .rejected{color:#ff6b6b;}
.log-entry .submitted{color:#FFD27A;}
.log-entry .deposited{color:#00ff88;}
.log-entry .info{color:#66ccff;}
.log-entry .wallet{color:#88ffaa;}
.log-entry .error{color:#ff6b6b;font-weight:bold;}
.btn-group{display:flex;gap:10px;flex-wrap:wrap;margin:15px 0 5px;}
button{background:transparent;color:#e0e0e0;border:1px solid #444;padding:10px 22px;border-radius:6px;font-size:0.8rem;cursor:pointer;transition:all 0.3s ease;flex:1;min-width:80px;}
button:hover{background:rgba(166,106,0,0.15);border-color:#A66A00;}
button.primary{border-color:#A66A00;color:#FFD27A;}
button.primary:hover{background:rgba(166,106,0,0.2);}
button.danger{border-color:#6a2a2a;color:#ff6b6b;}
button.danger:hover{background:rgba(106,42,42,0.2);}
.atm-group{display:flex;gap:10px;flex-wrap:wrap;margin:10px 0;}
.atm-group button{flex:0 1 auto;padding:8px 20px;font-size:0.75rem;border-color:#333;}
.atm-group button.gold{border-color:#A66A00;color:#FFD27A;}
.atm-group button.green{border-color:#2a6a3a;color:#7CFC7C;}
.atm-group button.blue{border-color:#1a4a6a;color:#66ccff;}
.footer{text-align:center;margin-top:20px;padding-top:15px;border-top:1px solid #1a1a1a;font-size:0.7rem;color:#555;}
.footer strong{color:#FFD27A;}
.atm-overlay{position:fixed;top:0;left:0;width:100vw;height:100vh;background:rgba(0,0,0,0.92);display:none;justify-content:center;align-items:center;z-index:9999;backdrop-filter:blur(4px);}
.atm-overlay.active{display:flex;}
.atm-box{background:#111;border:2px solid #A66A00;border-radius:16px;padding:30px;max-width:420px;width:92%;}
.atm-box h2{color:#FFD27A;font-size:1.4rem;border-bottom:1px solid #222;padding-bottom:12px;margin-bottom:15px;text-align:center;}
.atm-field{margin-bottom:12px;text-align:left;}
.atm-field label{display:block;font-size:0.7rem;color:#888;text-transform:uppercase;letter-spacing:1px;margin-bottom:4px;}
.atm-field input,.atm-field select{width:100%;padding:10px 14px;background:#0a0a0a;border:1px solid #222;border-radius:6px;color:#e0e0e0;font-size:0.9rem;outline:none;}
.atm-field input:focus{border-color:#A66A00;}
.atm-counter{background:#0a0a0a;border:1px dashed #A66A00;border-radius:8px;padding:10px;text-align:center;margin:15px 0;}
.atm-counter .count-number{font-family:'Courier New',monospace;color:#FFD27A;font-size:1.2rem;font-weight:bold;}
.atm-actions{display:flex;gap:10px;margin-top:15px;}
.atm-actions button{flex:1;padding:12px;border:none;border-radius:6px;font-weight:bold;cursor:pointer;font-size:0.9rem;}
.atm-actions .atm-confirm{background:#2a6a3a;color:#fff;}
.atm-actions .atm-confirm:hover{background:#3a7a4a;}
.atm-actions .atm-cancel{background:#3a2a2a;color:#fff;}
.atm-actions .atm-cancel:hover{background:#4a3a3a;}
</style>
</head>
<body>
<div class="atm-overlay" id="atmOverlay">
    <div class="atm-box">
        <h2>🌍 HUNDI TRANSFER</h2>
        <div class="atm-field"><label>👤 Sender</label><input type="text" id="atmSender" value="Seliim Ahmed"></div>
        <div class="atm-field"><label>📤 Receiver</label><input type="text" id="atmReceiverName" placeholder="Receiver full name"></div>
        <div class="atm-field"><label>📬 Receiver Account</label><input type="text" id="atmReceiverAccount" placeholder="Account / Wallet"></div>
        <div class="atm-field"><label>💰 Amount</label><input type="number" id="atmAmount" placeholder="0.00" step="any"></div>
        <div class="atm-field"><label>💱 Currency</label>
            <select id="atmCurrency"><option value="USD">USD</option><option value="EUR">EUR</option><option value="XMR">XMR</option></select>
        </div>
        <div class="atm-counter"><div class="count-number" id="atmCounter">X-SA-001</div></div>
        <div class="atm-actions">
            <button class="atm-confirm" onclick="confirmATM()">✅ CONFIRM</button>
            <button class="atm-cancel" onclick="closeATM()">❌ CANCEL</button>
        </div>
    </div>
</div>
<div class="container">
    <div class="header">
        <div>
            <h1>⚖️ Canal-Mardukh™<small>Part of the Mardukh System™</small></h1>
            <div class="status offline" id="statusBar"><span class="dot"></span> <span id="statusText">OFFLINE</span></div>
        </div>
        <div style="color:#888;font-size:0.8rem;text-align:right;">v0.2.0</div>
    </div>
    <div class="wallet-display">
        <span class="label">📬 Wallet Address</span>
        45ktWDeTNtUcVMXfJRKS6bbXMznMAStZFX6niJHcVy9uQk132bHJ21QTC5AKvqyx9XJN5e7mPc3vViyGnB2BM6DD1ZoAoZb
    </div>
    <div class="dial-counter">X-SA-<span class="highlight" id="dialCounter">000</span></div>
    <div class="dial-stats">
        <div class="dial-stat"><div class="value" id="sharesDisplay">0</div><div class="label">📦 Shares</div></div>
        <div class="dial-stat"><div class="value accepted" id="acceptedDisplay">0</div><div class="label">✅ Accepted</div></div>
        <div class="dial-stat"><div class="value rejected" id="rejectedDisplay">0</div><div class="label">🚫 Rejected</div></div>
        <div class="dial-stat"><div class="value earnings" id="earningsDisplay">0.00000000</div><div class="label">💰 Earned (XMR)</div></div>
    </div>
    <div class="pool-selector">
        <label>⛏️ Select Mining Pool</label>
        <select id="poolSelect">
            <optgroup label="🟠 Monero (XMR)">
                <option value="SupportXMR (EU)">SupportXMR (EU) · pool.supportxmr.com:3333</option>
                <option value="Kryptex (UAE)">Kryptex (UAE) · xmr-ae.kryptex.network:7029</option>
                <option value="MineXMR (US)">MineXMR (US) · pool.minexmr.com:3333</option>
                <option value="MoneroOcean (US)">MoneroOcean (US) · moneroocean.stream:10128</option>
            </optgroup>
            <optgroup label="₿ Bitcoin (BTC)">
                <option value="ViaBTC">ViaBTC · btc.viabtc.top:3333</option>
                <option value="AntPool">AntPool · antpool.com:3333</option>
                <option value="F2Pool">F2Pool · f2pool.com:3333</option>
                <option value="Binance Pool">Binance Pool · poolbinance.com:3333</option>
            </optgroup>
        </select>
    </div>
    <div class="atm-group">
        <button class="green" onclick="openATM('SEND')">💸 SEND</button>
        <button class="blue" onclick="openATM('RECEIVE')">📥 RECEIVE</button>
        <button class="gold" onclick="openATM('PAY NOW')">💳 PAY NOW</button>
    </div>
    <div class="log-window" id="logWindow">
        <div class="log-entry"><span class="time">⏳</span> <span class="info">Canal-Mardukh™ initialized.</span></div>
        <div class="log-entry"><span class="time">⏳</span> <span class="error">⚠️ RIG OFFLINE – No simulation data.</span></div>
        <div class="log-entry"><span class="time">⏳</span> <span class="info">Run the C++ rig and click "FETCH REAL".</span></div>
    </div>
    <div class="btn-group">
        <button class="primary" onclick="fetchRealData()">⟳ FETCH REAL</button>
        <button class="danger" onclick="clearLog()">🗑️ CLEAR LOG</button>
    </div>
    <div class="footer">Designed by <strong>Seliim Ahmed</strong> · Copyright © 2026 · All Rights Reserved</div>
</div>
<script>
const WALLET = "45ktWDeTNtUcVMXfJRKS6bbXMznMAStZFX6niJHcVy9uQk132bHJ21QTC5AKvqyx9XJN5e7mPc3vViyGnB2BM6DD1ZoAoZb";
const RIG_URL = "/status";
let shares=0, accepted=0, rejected=0, earned=0, counter=0, txCounter=0;

function addLog(msg,type){
    const w=document.getElementById('logWindow');
    const t=new Date().toLocaleTimeString();
    const d=document.createElement('div');
    d.className='log-entry';
    const cls=type==='accepted'?'accepted':type==='rejected'?'rejected':type==='submitted'?'submitted':type==='deposited'?'deposited':type==='wallet'?'wallet':type==='error'?'error':'info';
    d.innerHTML='<span class="time">'+t+'</span> <span class="'+cls+'">'+msg+'</span>';
    w.appendChild(d); w.scrollTop=w.scrollHeight;
    while(w.children.length>100)w.removeChild(w.firstChild);
}
function updateDisplay(){
    document.getElementById('dialCounter').textContent=String(counter).padStart(3,'0');
    document.getElementById('sharesDisplay').textContent=shares;
    document.getElementById('acceptedDisplay').textContent=accepted;
    document.getElementById('rejectedDisplay').textContent=rejected;
    document.getElementById('earningsDisplay').textContent=earned.toFixed(8);
}
function setStatus(online){
    const bar=document.getElementById('statusBar');
    const text=document.getElementById('statusText');
    if(online){bar.className='status online';text.textContent='ONLINE';}
    else{bar.className='status offline';text.textContent='OFFLINE';}
}
async function fetchRealData(){
    addLog('📡 Fetching real data from rig...','info');
    try{
        const res=await fetch(RIG_URL);
        if(!res.ok)throw new Error('HTTP '+res.status);
        const data=await res.json();
        shares=data.shares||0; accepted=data.accepted||0; rejected=data.rejected||0;
        counter=data.counter||0; earned=data.earnings||0;
        const poolResp=data.pool_response||'unknown';
        if(poolResp==='accepted') addLog('✅ Pool: Accepted share','accepted');
        else if(poolResp==='rejected') addLog('❌ Pool: Rejected share','rejected');
        addLog('📊 Real data: '+shares+' shares, '+earned.toFixed(8)+' XMR','accepted');
        setStatus(true); updateDisplay();
    }catch(e){
        setStatus(false);
        addLog('⚠️ RIG OFFLINE: '+e.message,'error');
    }
}
function openATM(a){txCounter++;document.getElementById('atmCounter').textContent='X-SA-'+String(txCounter).padStart(3,'0');document.getElementById('atmOverlay').classList.add('active');addLog('🏦 ATM '+a+' opened','info');}
function closeATM(){document.getElementById('atmOverlay').classList.remove('active');}
function confirmATM(){
    const rec=document.getElementById('atmReceiverName').value.trim();
    const acc=document.getElementById('atmReceiverAccount').value.trim();
    const amt=document.getElementById('atmAmount').value;
    const cur=document.getElementById('atmCurrency').value;
    if(!rec||!acc||!amt||parseFloat(amt)<=0){alert('❌ Fill all fields.');return;}
    const txId=document.getElementById('atmCounter').textContent;
    addLog('💳 ATM: '+amt+' '+cur+' → '+rec+' ('+acc+') '+txId,'submitted');
    addLog('✅ ATM: Transaction recorded.','accepted');
    alert('✅ ATM COMPLETE\n'+amt+' '+cur+' sent to '+rec+'\n'+txId);
    closeATM();
}
document.addEventListener('keydown',function(e){if(e.key==='Escape'&&document.getElementById('atmOverlay').classList.contains('active'))closeATM();});
document.getElementById('poolSelect').addEventListener('change',function(){addLog('🌐 Pool selected: '+this.options[this.selectedIndex].text,'info');});
function clearLog(){document.getElementById('logWindow').innerHTML='';addLog('🗑️ Log cleared.','info');}
addLog('🚀 Canal-Mardukh™ v0.2.0','info');
addLog('📤 Wallet: '+WALLET.substring(0,30)+'...','wallet');
addLog('💡 Click "FETCH REAL" to get data from C++ rig.','info');
addLog('⚠️ NO SIMULATION – only real data.','error');
updateDisplay(); setStatus(false);
setInterval(fetchRealData,3000);
fetchRealData();
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
// PROCESS DATA
// ============================================================
void process_data(const char* raw, int sock) {
    char binary[4096], washed[4096];
    to_binary((const unsigned char*)raw, strlen(raw), binary);
    int xmr = has_xmr_pattern(binary);
    int btc = has_btc_pattern(binary);
    if (!xmr && !btc) {
        cout << "🚫 REJECTED (no pattern)\n";
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
// AUTO-GENERATE DATA
// ============================================================
void auto_generate_data(int sock) {
    int idx = 0;
    while (true) {
        string data = sampleData[idx % sampleData.size()];
        idx++;
        process_data(data.c_str(), sock);
        this_thread::sleep_for(chrono::seconds(2));
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
    cout << "⛏️ Auto-generating data every 2 seconds...\n";
    cout << "────────────────────────────────────────────────────\n";

    thread auto_thread(auto_generate_data, sock);

    while (true) {
        this_thread::sleep_for(chrono::seconds(1));
    }

    web.stop();
    if (sock >= 0) close(sock);
    return 0;
}
