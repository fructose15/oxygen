#include "ide_server.hpp"
#include "interpreter.hpp"
#include "oxygen.h"

#include <algorithm>
#include <cctype>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <mutex>
#include <sstream>
#include <string>
#include <thread>
#include <vector>

#ifdef _WIN32
typedef SOCKET oxy_sock_t;
#  define OXY_INVALID_SOCK INVALID_SOCKET
#  define oxy_close_sock closesocket
#else
#  include <arpa/inet.h>
#  include <netinet/in.h>
#  include <sys/socket.h>
#  include <unistd.h>
typedef int oxy_sock_t;
#  define OXY_INVALID_SOCK (-1)
#  define oxy_close_sock close
#endif

namespace fs = std::filesystem;

namespace oxygen {

static std::string json_escape(const std::string &s) {
    std::string out;
    out.reserve(s.size() + 16);
    for (unsigned char c : s) {
        switch (c) {
            case '"': out += "\\\""; break;
            case '\\': out += "\\\\"; break;
            case '\b': out += "\\b"; break;
            case '\f': out += "\\f"; break;
            case '\n': out += "\\n"; break;
            case '\r': out += "\\r"; break;
            case '\t': out += "\\t"; break;
            default:
                if (c < 0x20) {
                    char buf[8];
                    snprintf(buf, sizeof(buf), "\\u%04x", c);
                    out += buf;
                } else {
                    out += static_cast<char>(c);
                }
        }
    }
    return out;
}

static std::string extract_json_field(const std::string &body, const std::string &key) {
    std::string needle = "\"" + key + "\"";
    size_t pos = body.find(needle);
    if (pos == std::string::npos) return "";
    pos = body.find(':', pos + needle.size());
    if (pos == std::string::npos) return "";
    pos++;
    while (pos < body.size() && std::isspace(static_cast<unsigned char>(body[pos]))) pos++;
    if (pos >= body.size() || body[pos] != '"') return "";
    pos++;
    std::string val;
    while (pos < body.size()) {
        char ch = body[pos++];
        if (ch == '\\' && pos < body.size()) {
            char esc = body[pos++];
            switch (esc) {
                case 'n': val += '\n'; break;
                case 'r': val += '\r'; break;
                case 't': val += '\t'; break;
                case '"': val += '"'; break;
                case '\\': val += '\\'; break;
                default: val += esc; break;
            }
        } else if (ch == '"') {
            break;
        } else {
            val += ch;
        }
    }
    return val;
}

static std::string url_decode(const std::string &s) {
    std::string out;
    for (size_t i = 0; i < s.size(); ++i) {
        if (s[i] == '%' && i + 2 < s.size()) {
            int hex = 0;
            std::istringstream iss(s.substr(i + 1, 2));
            if (iss >> std::hex >> hex) {
                out += static_cast<char>(hex);
                i += 2;
                continue;
            }
        } else if (s[i] == '+') {
            out += ' ';
            continue;
        }
        out += s[i];
    }
    return out;
}

static std::string get_query_param(const std::string &target, const std::string &key) {
    size_t qpos = target.find('?');
    if (qpos == std::string::npos) return "";
    std::string qs = target.substr(qpos + 1);
    std::stringstream ss(qs);
    std::string pair;
    while (std::getline(ss, pair, '&')) {
        size_t eq = pair.find('=');
        if (eq != std::string::npos && pair.substr(0, eq) == key) {
            return url_decode(pair.substr(eq + 1));
        }
    }
    return "";
}

static std::string read_text_safe(const std::string &path) {
    std::ifstream in(path);
    if (!in.is_open()) return "";
    std::ostringstream ss;
    ss << in.rdbuf();
    return ss.str();
}

static std::string build_ide_html() {
    return R"OXY_HTML(<!doctype html>
<html lang="en">
<head>
<meta charset="utf-8" />
<meta name="viewport" content="width=device-width, initial-scale=1.0" />
<title>Oxygen IDE</title>
<style>
  :root {
    --bg: #f8fafc; --panel: #ffffff; --border: #e2e8f0; --text: #0f172a;
    --muted: #64748b; --accent: #1e3a8a; --green: #4d8b31; --code-bg: #ffffff;
  }
  html.dark {
    --bg: #0b1120; --panel: #111827; --border: #1f2937; --text: #f1f5f9;
    --muted: #94a3b8; --accent: #2563eb; --green: #6ba543; --code-bg: #0b1120;
  }
  * { box-sizing: border-box; }
  body { margin: 0; font-family: -apple-system, BlinkMacSystemFont, "Segoe UI", sans-serif; background: var(--bg); color: var(--text); height: 100vh; display: flex; flex-direction: column; }
  header { display: flex; align-items: center; justify-content: space-between; gap: 16px; padding: 10px 20px; background: var(--panel); border-bottom: 1px solid var(--border); }
  .brand { display: flex; align-items: center; gap: 10px; font-weight: 700; font-size: 18px; }
  .nav { display: flex; gap: 16px; }
  .nav button { background: none; border: none; border-bottom: 2px solid transparent; padding: 6px 2px; font-size: 14px; font-weight: 500; color: var(--muted); cursor: pointer; }
  .nav button.active { color: var(--text); border-bottom-color: var(--green); }
  .btn { padding: 7px 14px; border-radius: 8px; border: 1px solid var(--border); background: var(--panel); color: var(--text); font-size: 12px; font-weight: 600; cursor: pointer; }
  .btn-primary { background: var(--green); border-color: var(--green); color: #fff; }
  .btn-blue { background: var(--accent); border-color: var(--accent); color: #fff; }
  .main { flex: 1; display: flex; min-height: 0; overflow: hidden; }
  .sidebar { width: 240px; border-right: 1px solid var(--border); background: var(--panel); padding: 12px; display: flex; flex-direction: column; gap: 6px; overflow-y: auto; }
  .sidebar h4 { margin: 8px 0 4px; font-size: 11px; color: var(--muted); text-transform: uppercase; letter-spacing: 0.04em; }
  .file-btn { text-align: left; padding: 8px 10px; border-radius: 6px; border: none; background: transparent; color: var(--text); font-family: ui-monospace, SFMono-Regular, Menlo, monospace; font-size: 12px; cursor: pointer; }
  .file-btn.active { background: rgba(107, 165, 67, 0.14); color: var(--green); font-weight: 600; }
  .workspace { flex: 1; display: flex; flex-direction: column; min-width: 0; }
  .toolbar { display: flex; align-items: center; justify-content: space-between; gap: 10px; padding: 8px 16px; background: var(--panel); border-bottom: 1px solid var(--border); flex-wrap: wrap; }
  textarea { flex: 1; width: 100%; border: none; outline: none; resize: none; padding: 16px; font-family: ui-monospace, SFMono-Regular, Menlo, monospace; font-size: 13px; line-height: 1.6; background: var(--code-bg); color: var(--text); }
  .console { height: 220px; border-top: 1px solid var(--border); background: var(--panel); display: flex; flex-direction: column; }
  .console-head { display: flex; align-items: center; justify-content: space-between; padding: 6px 16px; border-bottom: 1px solid var(--border); font-size: 12px; font-weight: 600; color: var(--muted); }
  pre { margin: 0; padding: 12px 16px; font-family: ui-monospace, SFMono-Regular, Menlo, monospace; font-size: 12px; overflow: auto; flex: 1; white-space: pre-wrap; }
  .view-panel { flex: 1; padding: 24px; overflow-y: auto; max-width: 1040px; margin: 0 auto; width: 100%; }
  .cmd-row { padding: 12px 0; border-bottom: 1px solid var(--border); display: flex; justify-content: space-between; gap: 12px; align-items: center; }
  input[type="search"] { width: 100%; max-width: 380px; padding: 8px 12px; border-radius: 8px; border: 1px solid var(--border); background: var(--panel); color: var(--text); font-size: 13px; }
  footer { display: flex; justify-content: space-between; padding: 6px 20px; font-size: 12px; color: var(--muted); border-top: 1px solid var(--border); background: var(--panel); }
  @media (max-width: 768px) { .sidebar { display: none; } }
</style>
</head>
<body>
<header>
  <div class="brand">
    <svg width="28" height="28" viewBox="0 0 100 100" aria-hidden="true">
      <circle cx="50" cy="50" r="46" fill="#fff" stroke="#6BA543" stroke-width="2.5"/>
      <circle cx="50" cy="50" r="31" fill="#6BA543"/>
      <circle cx="83" cy="21" r="10" fill="#6BA543" stroke="#0C238E" stroke-width="2"/>
      <circle cx="67" cy="36" r="8" fill="#fff"/>
      <text x="35" y="61" fill="#0C238E" font-family="sans-serif" font-weight="700" font-size="34">O</text>
      <text x="58" y="69" fill="#0C238E" font-family="sans-serif" font-weight="700" font-size="20">2</text>
    </svg>
    <span>Oxygen</span>
  </div>
  <nav class="nav">
    <button class="active" onclick="showTab('ide', this)">IDE</button>
    <button onclick="showTab('commands', this)">Commands</button>
    <button onclick="showTab('docs', this)">Docs &amp; Headers</button>
  </nav>
  <button class="btn" onclick="document.documentElement.classList.toggle('dark')">Theme</button>
</header>

<div id="tab-ide" class="main">
  <aside class="sidebar">
    <h4>Scripts (.o2)</h4>
    <button class="file-btn active" onclick="loadSample('multithread', this)">multithread_workers.o2</button>
    <button class="file-btn" onclick="loadSample('hello', this)">hello.o2</button>
    <button class="file-btn" onclick="loadSample('channels', this)">channel_pipeline.o2</button>
    <h4>C/C++ Include</h4>
    <button class="file-btn" onclick="openDoc('src/oxygen.h')">src/oxygen.h</button>
    <button class="file-btn" onclick="openDoc('configure')">configure</button>
    <button class="file-btn" onclick="openDoc('config.h')">config.h</button>
  </aside>
  <div class="workspace">
    <div class="toolbar">
      <span id="current-file" style="font-family:monospace;font-size:13px;font-weight:600;">multithread_workers.o2</span>
      <div style="display:flex;gap:8px;">
        <button class="btn" onclick="compileScript()">Compile Binary</button>
        <button class="btn btn-primary" onclick="runScript()">Run Script</button>
      </div>
    </div>
    <textarea id="editor" spellcheck="false"></textarea>
    <div class="console">
      <div class="console-head">
        <span>Native Oxygen Output</span>
        <button class="btn" style="padding:3px 8px;" onclick="document.getElementById('out').textContent=''">Clear</button>
      </div>
      <pre id="out">Oxygen local IDE ready. Click Run Script to execute natively on your machine.</pre>
    </div>
  </div>
</div>

<div id="tab-commands" class="view-panel" style="display:none;">
  <h2 style="margin-top:0;">Oxygen Standard Command Catalog</h2>
  <p id="cat-meta" style="font-size:13px;color:var(--muted);">Loading catalog...</p>
  <input id="cmd-q" type="search" placeholder="Search commands (thread, mutex, channel, matrix...)" oninput="searchCommands()" />
  <div id="cmd-list" style="margin-top:16px;"></div>
</div>

<div id="tab-docs" class="view-panel" style="display:none;">
  <div style="display:flex;gap:8px;flex-wrap:wrap;margin-bottom:16px;">
    <button class="btn" onclick="openDoc('README.md')">README.md</button>
    <button class="btn" onclick="openDoc('MANUAL.md')">MANUAL.md</button>
    <button class="btn" onclick="openDoc('src/oxygen.h')">src/oxygen.h</button>
    <button class="btn" onclick="openDoc('configure')">configure</button>
    <button class="btn" onclick="openDoc('config.h')">config.h</button>
    <button class="btn" onclick="openDoc('LICENSE')">LICENSE</button>
    <button class="btn" onclick="openDoc('agents.md')">agents.md</button>
  </div>
  <pre id="doc-content" style="border:1px solid var(--border);border-radius:8px;padding:16px;background:var(--panel);"></pre>
</div>

<footer>
  <span>Oxygen (.o2) &middot; Created by Fructose</span>
  <a href="https://github.com/fructose15/oxygen" target="_blank" rel="noreferrer" style="color:inherit;">github.com/fructose15/oxygen</a>
</footer>

<script>
const SAMPLES = {
  multithread: `use module threading\n\nprint text "Launching parallel Oxygen worker pool..."\ncreate mutex console_guard\ncreate atomic counter completed_jobs with value 0\n\ndefine task process_batch with parameter batch_label\n    sleep for 40 milliseconds\n    lock mutex console_guard\n    print text "Worker finished batch: " plus batch_label\n    unlock mutex console_guard\n    increment atomic counter completed_jobs\nend task\n\nrun task process_batch with "North" in background as t1\nrun task process_batch with "South" in background as t2\nrun task process_batch with "East" in background as t3\nwait for all background tasks to finish\n\nread atomic counter completed_jobs into total\nprint text "All concurrent batches complete. Total: " plus total\n`,
  hello: `use module core\n\nprint text "Welcome to Oxygen!"\nset text creator to "Fructose"\nprint text "Creator: " plus creator\n`,
  channels: `use module threading\n\ncreate channel inbox\nsend text "Message 1" to channel inbox\nsend text "Message 2" to channel inbox\nreceive message from channel inbox into a\nreceive message from channel inbox into b\nprint text "Received: " plus a plus ", " plus b\n`
};
document.getElementById('editor').value = SAMPLES.multithread;

function loadSample(k, btn) {
  document.querySelectorAll('.file-btn').forEach(b => b.classList.remove('active'));
  if (btn) btn.classList.add('active');
  document.getElementById('current-file').textContent = k + '.o2';
  document.getElementById('editor').value = SAMPLES[k];
}

function showTab(id, btn) {
  document.querySelectorAll('.nav button').forEach(b => b.classList.remove('active'));
  if (btn) btn.classList.add('active');
  document.getElementById('tab-ide').style.display = id === 'ide' ? 'flex' : 'none';
  document.getElementById('tab-commands').style.display = id === 'commands' ? 'block' : 'none';
  document.getElementById('tab-docs').style.display = id === 'docs' ? 'block' : 'none';
  if (id === 'commands') searchCommands();
  if (id === 'docs' && !document.getElementById('doc-content').textContent) openDoc('README.md');
}

async function runScript() {
  const out = document.getElementById('out');
  out.textContent = 'Running natively...';
  const res = await fetch('/api/run', {
    method: 'POST',
    headers: { 'Content-Type': 'application/json' },
    body: JSON.stringify({ source: document.getElementById('editor').value })
  });
  const data = await res.json();
  out.textContent = data.stdout || data.stderr || '(Completed with no output)';
}

async function compileScript() {
  const out = document.getElementById('out');
  out.textContent = 'Compiling native binary with <oxygen.h>...';
  const name = document.getElementById('current-file').textContent.replace(/\.o2$/, '') || 'program';
  const res = await fetch('/api/compile', {
    method: 'POST',
    headers: { 'Content-Type': 'application/json' },
    body: JSON.stringify({ source: document.getElementById('editor').value, outputName: name })
  });
  const data = await res.json();
  out.textContent = data.stdout || data.stderr || 'Compilation complete';
}

async function searchCommands() {
  const q = document.getElementById('cmd-q').value || '';
  const res = await fetch('/api/commands?q=' + encodeURIComponent(q));
  const data = await res.json();
  document.getElementById('cat-meta').textContent = data.total + ' matching commands · Catalog size: ' + (data.bytes / (1024*1024)).toFixed(2) + ' MB';
  const el = document.getElementById('cmd-list');
  el.innerHTML = '';
  for (const item of data.items) {
    const row = document.createElement('div');
    row.className = 'cmd-row';
    row.innerHTML = '<div><strong>' + item.name + '</strong> &middot; <span style="color:var(--muted);font-size:12px;">' + item.category + '</span><div style="font-family:monospace;font-size:12px;color:var(--accent);margin:4px 0;">' + item.syntax.replace(/</g,'&lt;') + '</div><div style="font-size:12px;color:var(--muted);">' + item.description + '</div></div>';
    const btn = document.createElement('button');
    btn.className = 'btn';
    btn.textContent = 'Insert';
    btn.onclick = () => {
      document.getElementById('editor').value += '\n' + item.syntax + '\n';
      showTab('ide', document.querySelectorAll('.nav button')[0]);
    };
    row.appendChild(btn);
    el.appendChild(row);
  }
}

async function openDoc(p) {
  showTab('docs', document.querySelectorAll('.nav button')[2]);
  const res = await fetch('/api/doc?path=' + encodeURIComponent(p));
  const data = await res.json();
  document.getElementById('doc-content').textContent = data.content || 'Document not found.';
}
</script>
</body>
</html>
)OXY_HTML";
}

static void send_http_response(oxy_sock_t client,
                               const std::string &status,
                               const std::string &content_type,
                               const std::string &body) {
    std::ostringstream hdr;
    hdr << "HTTP/1.1 " << status << "\r\n"
        << "Content-Type: " << content_type << "\r\n"
        << "Content-Length: " << body.size() << "\r\n"
        << "Connection: close\r\n"
        << "Access-Control-Allow-Origin: *\r\n\r\n";
    std::string header_str = hdr.str();
    send(client, header_str.data(), static_cast<int>(header_str.size()), 0);
    if (!body.empty()) {
        send(client, body.data(), static_cast<int>(body.size()), 0);
    }
}

static void handle_http_client(oxy_sock_t client,
                               const std::string &oxygen_root,
                               const std::string &oxygen_home) {
    std::string request;
    char buf[4096];
    while (true) {
        int n = recv(client, buf, sizeof(buf), 0);
        if (n <= 0) break;
        request.append(buf, static_cast<size_t>(n));
        size_t hdr_end = request.find("\r\n\r\n");
        if (hdr_end != std::string::npos) {
            size_t content_len = 0;
            std::string lower_hdr = request.substr(0, hdr_end);
            for (char &c : lower_hdr) c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
            size_t cl_pos = lower_hdr.find("content-length:");
            if (cl_pos != std::string::npos) {
                content_len = static_cast<size_t>(std::strtoull(lower_hdr.c_str() + cl_pos + 15, nullptr, 10));
            }
            if (request.size() >= hdr_end + 4 + content_len) {
                break;
            }
        }
    }

    size_t line_end = request.find("\r\n");
    if (line_end == std::string::npos) {
        oxy_close_sock(client);
        return;
    }

    std::istringstream req_line(request.substr(0, line_end));
    std::string method, target;
    req_line >> method >> target;

    size_t hdr_end = request.find("\r\n\r\n");
    std::string body = (hdr_end != std::string::npos) ? request.substr(hdr_end + 4) : "";

    if (method == "GET" && (target == "/" || target.rfind("/?", 0) == 0)) {
        send_http_response(client, "200 OK", "text/html; charset=utf-8", build_ide_html());
    } else if (method == "POST" && target == "/api/run") {
        std::string source = extract_json_field(body, "source");
        std::ostringstream captured;
        std::mutex cap_mtx;
        Interpreter interp;
        interp.set_base_dir(oxygen_root);
        interp.load_catalog(oxygen_root + "/commands/oxygen_commands.o2cat");
        interp.set_output_sink([&](const std::string &line) {
            std::lock_guard<std::mutex> lock(cap_mtx);
            captured << line << "\n";
        });
        int rc = interp.execute_script(source);
        std::string json = "{\"exitCode\":" + std::to_string(rc) +
                           ",\"stdout\":\"" + json_escape(captured.str()) + "\",\"stderr\":\"\"}";
        send_http_response(client, "200 OK", "application/json", json);
    } else if (method == "POST" && target == "/api/compile") {
        std::string source = extract_json_field(body, "source");
        std::string out_name = extract_json_field(body, "outputName");
        if (out_name.empty()) out_name = "program";
        std::string tmp_dir = oxygen_home + "/tmp";
        fs::create_directories(tmp_dir);
        std::string script_file = tmp_dir + "/" + out_name + ".o2";
        std::string out_bin = tmp_dir + "/" + out_name;
        {
            std::ofstream out(script_file);
            out << source;
        }
        int rc = compile_script_to_binary(script_file, out_bin, oxygen_root);
        std::string msg = (rc == 0)
            ? ("Compiled native binary at: " + out_bin)
            : "Compilation failed (check that g++/clang++ is installed).";
        std::string json = "{\"exitCode\":" + std::to_string(rc) +
                           ",\"stdout\":\"" + json_escape(rc == 0 ? msg : "") +
                           "\",\"stderr\":\"" + json_escape(rc == 0 ? "" : msg) + "\"}";
        send_http_response(client, "200 OK", "application/json", json);
    } else if (method == "GET" && target.rfind("/api/commands", 0) == 0) {
        std::string q = get_query_param(target, "q");
        for (char &c : q) c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
        std::string cat_path = oxygen_root + "/commands/oxygen_commands.o2cat";
        uintmax_t file_bytes = fs::exists(cat_path) ? fs::file_size(cat_path) : 0;
        std::ifstream in(cat_path);
        std::string line;
        size_t total_matches = 0;
        std::ostringstream items_json;
        items_json << "[";
        size_t emitted = 0;
        while (std::getline(in, line)) {
            if (line.empty() || line[0] == '#') continue;
            std::string low = line;
            for (char &c : low) c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
            if (!q.empty() && low.find(q) == std::string::npos) continue;
            total_matches++;
            if (emitted < 50) {
                std::vector<std::string> cols;
                std::stringstream ss(line);
                std::string col;
                while (std::getline(ss, col, '\t')) cols.push_back(col);
                if (cols.size() >= 4) {
                    if (emitted > 0) items_json << ",";
                    items_json << "{\"name\":\"" << json_escape(cols[0])
                               << "\",\"category\":\"" << json_escape(cols[1])
                               << "\",\"syntax\":\"" << json_escape(cols[2])
                               << "\",\"description\":\"" << json_escape(cols[3]) << "\"}";
                    emitted++;
                }
            }
        }
        items_json << "]";
        std::string json = "{\"total\":" + std::to_string(total_matches) +
                           ",\"bytes\":" + std::to_string(file_bytes) +
                           ",\"items\":" + items_json.str() + "}";
        send_http_response(client, "200 OK", "application/json", json);
    } else if (method == "GET" && target.rfind("/api/doc", 0) == 0) {
        std::string p = get_query_param(target, "path");
        std::string content;
        if (p == "README.md" || p == "MANUAL.md" || p == "LICENSE" || p == "agents.md" ||
            p == "src/oxygen.h" || p == "configure" || p == "config.h") {
            content = read_text_safe(oxygen_root + "/" + p);
        }
        std::string json = "{\"content\":\"" + json_escape(content) + "\"}";
        send_http_response(client, "200 OK", "application/json", json);
    } else {
        send_http_response(client, "404 Not Found", "text/plain", "Not Found");
    }

    oxy_close_sock(client);
}

int start_ide_server(const std::string &oxygen_root, const std::string &oxygen_home) {
#ifdef _WIN32
    WSADATA wsa;
    if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0) {
        std::cerr << "Error: Failed to initialize Winsock." << std::endl;
        return 1;
    }
#endif

    oxy_sock_t srv = OXY_INVALID_SOCK;
    int bound_port = 7420;
    for (int port = 7420; port <= 7440; ++port) {
        oxy_sock_t s = socket(AF_INET, SOCK_STREAM, 0);
        if (s == OXY_INVALID_SOCK) continue;
        int opt = 1;
#ifdef _WIN32
        setsockopt(s, SOL_SOCKET, SO_REUSEADDR, reinterpret_cast<const char *>(&opt), sizeof(opt));
#else
        setsockopt(s, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
#endif
        sockaddr_in addr;
        std::memset(&addr, 0, sizeof(addr));
        addr.sin_family = AF_INET;
        addr.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
        addr.sin_port = htons(static_cast<uint16_t>(port));

        if (bind(s, reinterpret_cast<sockaddr *>(&addr), sizeof(addr)) == 0 &&
            listen(s, 32) == 0) {
            srv = s;
            bound_port = port;
            break;
        }
        oxy_close_sock(s);
    }

    if (srv == OXY_INVALID_SOCK) {
        std::cerr << "Error: Could not bind local Oxygen IDE server port." << std::endl;
#ifdef _WIN32
        WSACleanup();
#endif
        return 1;
    }

    std::string ide_url = "http://127.0.0.1:" + std::to_string(bound_port);
    std::cout << "Oxygen IDE (" << oxygen_detect_platform() << ") running at " << ide_url << std::endl;
    std::cout << "Press Ctrl+C to stop the IDE server." << std::endl;
    oxygen_open_browser_url(ide_url.c_str());

    while (true) {
        sockaddr_in client_addr;
#ifdef _WIN32
        int addr_len = sizeof(client_addr);
#else
        socklen_t addr_len = sizeof(client_addr);
#endif
        oxy_sock_t client = accept(srv, reinterpret_cast<sockaddr *>(&client_addr), &addr_len);
        if (client == OXY_INVALID_SOCK) continue;
        std::thread(handle_http_client, client, oxygen_root, oxygen_home).detach();
    }

    oxy_close_sock(srv);
#ifdef _WIN32
    WSACleanup();
#endif
    return 0;
}

} // namespace oxygen
