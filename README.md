<!doctype html>
<html lang="en">
<head>
  <meta charset="UTF-8" />
  <meta name="viewport" content="width=device-width, initial-scale=1.0" />
  <title>Oxygen — English-Syntax Multithreaded Programming Language</title>
  <meta name="description" content="Oxygen (.o2) is a C/C++ powered, English-like multithreaded programming language created by Fructose, with an integrated cross-platform IDE and 1 MB+ standard command library." />
  <meta property="og:title" content="Oxygen Programming Language (.o2)" />
  <meta property="og:description" content="Natural English syntax, built-in multithreading, native C/C++ compiler, and cross-platform IDE by Fructose." />
  <meta property="og:type" content="website" />
  <link rel="preconnect" href="https://fonts.googleapis.com" />
  <link rel="preconnect" href="https://fonts.gstatic.com" crossorigin />
  <link href="https://fonts.googleapis.com/css2?family=JetBrains+Mono:wght@400;500;600&family=Outfit:wght@500;600;700&family=Plus+Jakarta+Sans:wght@400;500;600;700&display=swap" rel="stylesheet" />
  <style>
    :root {
      --bg: #f8fafc;
      --surface: #ffffff;
      --surface-alt: #f1f5f9;
      --border: #e2e8f0;
      --text: #0f172a;
      --muted: #475569;
      --cobalt: #1e3a8a;
      --cobalt-hover: #1d4ed8;
      --leaf: #4d8b31;
      --leaf-soft: rgba(77, 139, 49, 0.12);
      --code-bg: #0f172a;
      --code-text: #f8fafc;
    }

    html.dark {
      --bg: #0b1120;
      --surface: #111827;
      --surface-alt: #1e293b;
      --border: #1f2937;
      --text: #f1f5f9;
      --muted: #94a3b8;
      --cobalt: #2563eb;
      --cobalt-hover: #3b82f6;
      --leaf: #6ba543;
      --leaf-soft: rgba(107, 165, 67, 0.16);
      --code-bg: #050914;
      --code-text: #e2e8f0;
    }

    * { box-sizing: border-box; }

    body {
      margin: 0;
      font-family: 'Plus Jakarta Sans', system-ui, -apple-system, sans-serif;
      background: var(--bg);
      color: var(--text);
      line-height: 1.6;
    }

    h1, h2, h3, .brand-name {
      font-family: 'Outfit', system-ui, sans-serif;
      letter-spacing: -0.02em;
    }

    code, pre, textarea, .mono {
      font-family: 'JetBrains Mono', ui-monospace, SFMono-Regular, Menlo, monospace;
      font-variant-numeric: tabular-nums;
    }

    a { color: inherit; text-decoration: none; }

    .container {
      max-width: 1180px;
      margin: 0 auto;
      padding: 0 24px;
    }

    /* Top Bar */
    header {
      position: sticky;
      top: 0;
      z-index: 20;
      background: var(--surface);
      border-bottom: 1px solid var(--border);
    }

    .topbar {
      display: flex;
      align-items: center;
      justify-content: space-between;
      gap: 24px;
      height: 60px;
    }

    .brand {
      display: inline-flex;
      align-items: center;
      gap: 10px;
      font-weight: 700;
      font-size: 18px;
      white-space: nowrap;
      flex-shrink: 0;
    }

    .nav-links {
      display: flex;
      align-items: center;
      gap: 24px;
    }

    .nav-links a {
      font-size: 14px;
      font-weight: 500;
      color: var(--muted);
      white-space: nowrap;
    }

    .nav-links a:hover {
      color: var(--text);
      text-decoration: underline;
      text-underline-offset: 6px;
    }

    .btn {
      display: inline-flex;
      align-items: center;
      justify-content: center;
      gap: 8px;
      padding: 8px 16px;
      border-radius: 8px;
      font-size: 13px;
      font-weight: 600;
      border: 1px solid var(--border);
      background: var(--surface);
      color: var(--text);
      cursor: pointer;
      white-space: nowrap;
      transition: background 150ms ease, border-color 150ms ease;
    }

    .btn:hover { background: var(--surface-alt); }

    .btn-primary {
      background: var(--cobalt);
      border-color: var(--cobalt);
      color: #ffffff;
    }

    .btn-primary:hover {
      background: var(--cobalt-hover);
      border-color: var(--cobalt-hover);
    }

    .btn-green {
      background: var(--leaf);
      border-color: var(--leaf);
      color: #ffffff;
    }

    .btn-green:hover {
      background: var(--leaf);
      border-color: var(--leaf);
      opacity: 0.9;
    }

    /* Hero */
    .hero {
      padding: 56px 0 40px;
      border-bottom: 1px solid var(--border);
    }

    .hero-grid {
      display: grid;
      grid-template-columns: 1.15fr 0.85fr;
      gap: 40px;
      align-items: center;
    }

    .hero h1 {
      margin: 0 0 16px;
      font-size: clamp(2rem, 4vw, 3.1rem);
      line-height: 1.12;
      text-wrap: balance;
    }

    .hero p {
      margin: 0 0 24px;
      font-size: 16px;
      color: var(--muted);
      max-width: 60ch;
    }

    .hero-actions {
      display: flex;
      flex-wrap: wrap;
      gap: 12px;
      margin-bottom: 20px;
    }

    .meta-line {
      font-size: 13px;
      color: var(--muted);
      display: flex;
      flex-wrap: wrap;
      gap: 10px;
    }

    .code-card {
      background: var(--code-bg);
      color: var(--code-text);
      border-radius: 10px;
      border: 1px solid var(--border);
      overflow: hidden;
    }

    .code-card-head {
      padding: 10px 16px;
      font-size: 12px;
      color: #94a3b8;
      border-bottom: 1px solid rgba(255, 255, 255, 0.08);
      display: flex;
      justify-content: space-between;
    }

    .code-card pre {
      margin: 0;
      padding: 16px;
      font-size: 13px;
      line-height: 1.65;
      overflow-x: auto;
    }

    /* Sections */
    section.block {
      padding: 48px 0;
      border-bottom: 1px solid var(--border);
    }

    .section-head {
      margin-bottom: 24px;
    }

    .section-head h2 {
      margin: 0 0 6px;
      font-size: 24px;
    }

    .section-head p {
      margin: 0;
      font-size: 14px;
      color: var(--muted);
    }

    /* Playground */
    .playground {
      background: var(--surface);
      border: 1px solid var(--border);
      border-radius: 10px;
      overflow: hidden;
    }

    .pg-toolbar {
      display: flex;
      flex-wrap: wrap;
      align-items: center;
      justify-content: space-between;
      gap: 12px;
      padding: 10px 16px;
      border-bottom: 1px solid var(--border);
      background: var(--surface-alt);
    }

    .pg-tabs {
      display: flex;
      gap: 6px;
      flex-wrap: wrap;
    }

    .pg-tab {
      padding: 6px 12px;
      border-radius: 6px;
      border: 1px solid transparent;
      background: transparent;
      color: var(--muted);
      font-size: 12px;
      font-weight: 600;
      cursor: pointer;
    }

    .pg-tab.active {
      background: var(--surface);
      border-color: var(--border);
      color: var(--text);
    }

    .pg-grid {
      display: grid;
      grid-template-columns: 1.2fr 0.8fr;
      min-height: 300px;
    }

    .pg-editor {
      width: 100%;
      min-height: 300px;
      padding: 16px;
      border: none;
      border-right: 1px solid var(--border);
      background: var(--surface);
      color: var(--text);
      font-size: 13px;
      line-height: 1.6;
      resize: vertical;
      outline: none;
    }

    .pg-output {
      padding: 16px;
      background: var(--bg);
      font-size: 13px;
      line-height: 1.6;
      margin: 0;
      overflow: auto;
      white-space: pre-wrap;
    }

    /* Build Matrix */
    .tabs-bar {
      display: inline-flex;
      gap: 4px;
      padding: 4px;
      background: var(--surface-alt);
      border-radius: 8px;
      margin-bottom: 16px;
      flex-wrap: wrap;
    }

    .tab-btn {
      padding: 6px 14px;
      border-radius: 6px;
      border: none;
      background: transparent;
      color: var(--muted);
      font-size: 13px;
      font-weight: 600;
      cursor: pointer;
    }

    .tab-btn.active {
      background: var(--surface);
      color: var(--text);
      box-shadow: 0 1px 2px rgba(0, 0, 0, 0.06);
    }

    /* CLI Table */
    .cli-table {
      width: 100%;
      border-collapse: collapse;
      background: var(--surface);
      border: 1px solid var(--border);
      border-radius: 10px;
      overflow: hidden;
      font-size: 13px;
    }

    .cli-table th, .cli-table td {
      text-align: left;
      padding: 10px 14px;
      border-bottom: 1px solid var(--border);
      vertical-align: top;
    }

    .cli-table th {
      background: var(--surface-alt);
      font-weight: 600;
      font-size: 12px;
      color: var(--muted);
    }

    .cli-table tr:last-child td { border-bottom: none; }

    /* Footer */
    footer {
      padding: 24px 0;
      font-size: 13px;
      color: var(--muted);
    }

    .footer-inner {
      display: flex;
      flex-wrap: wrap;
      align-items: center;
      justify-content: space-between;
      gap: 16px;
    }

    @media (max-width: 860px) {
      .hero-grid, .pg-grid { grid-template-columns: 1fr; }
      .pg-editor { border-right: none; border-bottom: 1px solid var(--border); min-height: 220px; }
      .nav-links { display: none; }
    }
  </style>
</head>
<body>
  <header>
    <div class="container topbar">
      <a href="#top" class="brand">
        <svg width="30" height="30" viewBox="0 0 100 100" aria-hidden="true">
          <circle cx="50" cy="50" r="46" fill="#ffffff" stroke="#6BA543" stroke-width="2.5" />
          <circle cx="50" cy="50" r="31" fill="#6BA543" />
          <circle cx="83" cy="21" r="10" fill="#6BA543" stroke="#0C238E" stroke-width="2" />
          <circle cx="67" cy="36" r="8" fill="#ffffff" />
          <text x="35" y="61" fill="#0C238E" font-family="sans-serif" font-weight="700" font-size="34">O</text>
          <text x="58" y="69" fill="#0C238E" font-family="sans-serif" font-weight="700" font-size="20">2</text>
        </svg>
        <span class="brand-name">Oxygen</span>
      </a>

      <nav class="nav-links" aria-label="Primary navigation">
        <a href="#playground">Playground</a>
        <a href="#build">Build &amp; Platforms</a>
        <a href="#cli">CLI Reference</a>
        <a href="#docs">Documentation</a>
      </nav>

      <a class="btn btn-primary" href="https://github.com/fructose15/oxygen" target="_blank" rel="noreferrer">
        View on GitHub
      </a>
    </div>
  </header>

  <main id="top">
    <section class="hero">
      <div class="container hero-grid">
        <div>
          <h1>Programming that reads like natural English, powered by C and C++.</h1>
          <p>
            Oxygen (<code>.o2</code>) combines readable English prose commands with native OS multithreading,
            channels, mutexes, atomic counters, a 1 MB+ standard command library, and an integrated cross-platform IDE.
          </p>
          <div class="hero-actions">
            <a class="btn btn-green" href="#playground">Try Live Playground</a>
            <a class="btn" href="#build">Build Instructions</a>
            <a class="btn" href="https://github.com/fructose15/oxygen" target="_blank" rel="noreferrer">Source Repository</a>
          </div>
          <div class="meta-line">
            <span>Creator: <strong>Fructose</strong></span>
            <span>&middot;</span>
            <span>Extension: <code>.o2</code></span>
            <span>&middot;</span>
            <span>Header: <code>#include &lt;oxygen.h&gt;</code></span>
            <span>&middot;</span>
            <span>macOS &middot; Linux &middot; Windows &middot; Android &middot; iOS</span>
          </div>
        </div>

        <div class="code-card">
          <div class="code-card-head">
            <span>multithread_workers.o2</span>
            <span>C/C++ Native Runtime</span>
          </div>
          <pre>use module threading

create mutex console_guard
create atomic counter done with value 0

define task worker with parameter label
    sleep for 40 milliseconds
    lock mutex console_guard
    print text "Finished: " plus label
    unlock mutex console_guard
    increment atomic counter done
end task

run task worker with "Alpha" in background as t1
run task worker with "Beta" in background as t2
wait for all background tasks to finish

read atomic counter done into total
print text "Jobs complete: " plus total</pre>
        </div>
      </div>
    </section>

    <section id="playground" class="block">
      <div class="container">
        <div class="section-head">
          <h2>Interactive Oxygen (.o2) Playground</h2>
          <p>Edit English-syntax Oxygen code below and execute concurrent tasks directly in your browser.</p>
        </div>

        <div class="playground">
          <div class="pg-toolbar">
            <div class="pg-tabs" role="tablist">
              <button type="button" class="pg-tab active" onclick="loadExample('threads', this)">multithread_workers.o2</button>
              <button type="button" class="pg-tab" onclick="loadExample('channels', this)">channel_pipeline.o2</button>
              <button type="button" class="pg-tab" onclick="loadExample('hello', this)">hello.o2</button>
            </div>
            <button type="button" class="btn btn-green" onclick="runOxygenPlayground()">Run Script</button>
          </div>
          <div class="pg-grid">
            <textarea id="pg-code" class="pg-editor" spellcheck="false" aria-label="Oxygen code editor"></textarea>
            <pre id="pg-out" class="pg-output">Click "Run Script" to execute the .o2 program.</pre>
          </div>
        </div>
      </div>
    </section>

    <section id="build" class="block">
      <div class="container">
        <div class="section-head">
          <h2>Cross-Platform Build &amp; Embedding</h2>
          <p>Use <code>./configure</code> to generate <code>Makefile</code> on POSIX targets, or <code>config.h</code> directly on Windows.</p>
        </div>

        <div class="tabs-bar" role="tablist">
          <button type="button" class="tab-btn active" onclick="selectPlatform('linux', this)">Linux</button>
          <button type="button" class="tab-btn" onclick="selectPlatform('macos', this)">macOS</button>
          <button type="button" class="tab-btn" onclick="selectPlatform('windows', this)">Windows (config.h)</button>
          <button type="button" class="tab-btn" onclick="selectPlatform('android', this)">Android</button>
          <button type="button" class="tab-btn" onclick="selectPlatform('ios', this)">iOS</button>
        </div>

        <div class="code-card">
          <div class="code-card-head">
            <span id="platform-title">Linux (GCC / Clang)</span>
            <span>Installs binary + include/oxygen.h</span>
          </div>
          <pre id="platform-cmd">chmod +x configure
./configure --prefix=/usr/local --target=linux
make
sudo make install
oxygen --ide</pre>
        </div>
      </div>
    </section>

    <section id="cli" class="block">
      <div class="container">
        <div class="section-head">
          <h2>Oxygen CLI Reference</h2>
          <p>Usage: <code>oxygen &lt;option&gt; [arg]</code></p>
        </div>

        <table class="cli-table">
          <thead>
            <tr>
              <th>Command</th>
              <th>Purpose</th>
            </tr>
          </thead>
          <tbody>
            <tr><td class="mono">oxygen --command &lt;command&gt;</td><td>Execute a single Oxygen language command directly</td></tr>
            <tr><td class="mono">oxygen --run &lt;script.o2&gt;</td><td>Run an Oxygen script file with the built-in interpreter</td></tr>
            <tr><td class="mono">oxygen --compile &lt;script.o2&gt; -o &lt;binary&gt;</td><td>Compile an <code>.o2</code> script into a standalone native binary with <code>&lt;oxygen.h&gt;</code></td></tr>
            <tr><td class="mono">oxygen --ide</td><td>Launch the cross-platform Oxygen IDE server</td></tr>
            <tr><td class="mono">oxygen --install-command &lt;name&gt;</td><td>Download and install an Oxygen command by name</td></tr>
            <tr><td class="mono">oxygen --install-module &lt;name&gt;</td><td>Download and install an Oxygen module by name</td></tr>
            <tr><td class="mono">oxygen --create-command &lt;file&gt; &lt;name&gt;</td><td>Publish your command publicly</td></tr>
            <tr><td class="mono">oxygen --create-module &lt;binary&gt; &lt;name&gt;</td><td>Publish your module publicly</td></tr>
            <tr><td class="mono">oxygen --delete-my-command &lt;name&gt;</td><td>Delete a command you published publicly</td></tr>
            <tr><td class="mono">oxygen --delete-my-module &lt;name&gt;</td><td>Delete a module you published publicly</td></tr>
            <tr><td class="mono">oxygen --delete-command &lt;name&gt;</td><td>Delete a locally owned command</td></tr>
            <tr><td class="mono">oxygen --delete-module &lt;name&gt;</td><td>Delete a locally owned module</td></tr>
            <tr><td class="mono">oxygen --save-command &lt;file&gt; &lt;name&gt;</td><td>Save a command locally into Oxygen</td></tr>
            <tr><td class="mono">oxygen --save-module &lt;binary&gt; &lt;name&gt;</td><td>Save a module locally into Oxygen</td></tr>
            <tr><td class="mono">oxygen --login / --logout</td><td>Sign in or out of your Oxygen account before publishing</td></tr>
            <tr><td class="mono">oxygen --update</td><td>Update Oxygen to the latest release</td></tr>
            <tr><td class="mono">oxygen --readme / --manual / --license / --agents</td><td>Print README, MANUAL, LICENSE, or agents.md</td></tr>
            <tr><td class="mono">oxygen --codeowner</td><td>Print the creator of Oxygen (<code>Fructose</code>)</td></tr>
            <tr><td class="mono">oxygen --repo</td><td>Open <code>https://github.com/fructose15/oxygen</code></td></tr>
            <tr><td class="mono">oxygen --help</td><td>Display all supported CLI options</td></tr>
          </tbody>
        </table>
      </div>
    </section>

    <section id="docs" class="block">
      <div class="container">
        <div class="section-head">
          <h2>Repository Structure &amp; Documentation</h2>
          <p>All source files, headers, and manuals live cleanly inside the Oxygen repository.</p>
        </div>
        <div class="code-card">
          <div class="code-card-head">
            <span>github.com/fructose15/oxygen</span>
            <span>MIT License &middot; Fructose</span>
          </div>
          <pre>oxygen/
├── configure                 # Generates Makefile for Linux, macOS, Android, iOS
├── config.h                  # Windows MSVC / MinGW configuration &amp; Win32 threading
├── src/
│   ├── oxygen.h              # Public C/C++ header (#include &lt;oxygen.h&gt;)
│   ├── runtime.c             # C multithreading, mutexes, channels, and atomics
│   ├── interpreter.hpp/cpp   # C++ English-syntax Oxygen interpreter
│   ├── compiler.cpp          # Native ahead-of-time .o2 binary compiler
│   ├── ide_server.hpp/cpp    # Built-in cross-platform local IDE server
│   └── main.cpp              # oxygen CLI entry point
├── commands/
│   └── oxygen_commands.o2cat # 1 MB+ English-syntax Standard Command Catalog
├── examples/                 # hello.o2, multithread_workers.o2, channel_pipeline.o2
├── README.md
├── MANUAL.md
├── LICENSE
├── agents.md
└── CODEOWNERS</pre>
        </div>
      </div>
    </section>
  </main>

  <footer>
    <div class="container footer-inner">
      <div>Oxygen Programming Language (<code>.o2</code>) &middot; Created by Fructose &middot; <a href="https://github.com/fructose15/oxygen" target="_blank" rel="noreferrer">github.com/fructose15/oxygen</a></div>
      <button type="button" class="btn" onclick="document.documentElement.classList.toggle('dark')">Toggle Theme</button>
    </div>
  </footer>

  <script>
    const EXAMPLES = {
      threads: `use module threading\n\nprint text "Launching parallel Oxygen worker pool..."\ncreate mutex console_guard\ncreate atomic counter completed_jobs with value 0\n\ndefine task process_batch with parameter batch_label\n    sleep for 35 milliseconds\n    lock mutex console_guard\n    print text "Worker finished batch: " plus batch_label\n    unlock mutex console_guard\n    increment atomic counter completed_jobs\nend task\n\nrun task process_batch with "North Cluster" in background as t1\nrun task process_batch with "South Cluster" in background as t2\nrun task process_batch with "East Cluster" in background as t3\nwait for all background tasks to finish\n\nread atomic counter completed_jobs into total\nprint text "All concurrent batches complete. Total: " plus total`,
      channels: `use module threading\nuse module channels\n\ncreate channel work_queue\ncreate atomic counter processed with value 0\n\ndefine task producer with parameter limit\n    repeat 3 times\n        set text item to "Packet #" plus iteration\n        send text item to channel work_queue\n        print text "Queued: " plus item\n        sleep for 20 milliseconds\n    end repeat\nend task\n\ndefine task consumer with parameter id\n    repeat 3 times\n        receive message from channel work_queue into msg\n        increment atomic counter processed\n        print text "Consumer " plus id plus " handled " plus msg\n    end repeat\nend task\n\nrun task producer with 3 in background as p1\nrun task consumer with "A" in background as c1\nwait for all background tasks to finish`,
      hello: `use module core\n\nprint text "Welcome to Oxygen!"\nset text creator to "Fructose"\nset number threads to 4\nprint text "Creator: " plus creator\nif threads is greater than 2 then\n    print text "Multithreaded execution ready."\nend if`
    };

    const PLATFORMS = {
      linux: {
        title: 'Linux (GCC / Clang)',
        cmd: 'chmod +x configure\n./configure --prefix=/usr/local --target=linux\nmake\nsudo make install\noxygen --ide'
      },
      macos: {
        title: 'macOS (Clang / Xcode CLT)',
        cmd: 'chmod +x configure\n./configure --prefix=/usr/local --target=macos\nmake\nsudo make install\noxygen --ide'
      },
      windows: {
        title: 'Windows (config.h + MSVC / MinGW)',
        cmd: 'cl /EHsc /O2 /Isrc /Fe:oxygen.exe src\\runtime.c src\\interpreter.cpp src\\compiler.cpp src\\ide_server.cpp src\\main.cpp ws2_32.lib\n:: Or MinGW:\ng++ -O2 -std=c++17 -Isrc src\\runtime.c src\\interpreter.cpp src\\compiler.cpp src\\ide_server.cpp src\\main.cpp -lws2_32 -o oxygen.exe'
      },
      android: {
        title: 'Android (Termux / NDK)',
        cmd: 'pkg install clang make\n./configure --prefix=$PREFIX --target=android\nmake\nmake install\noxygen --ide'
      },
      ios: {
        title: 'iOS (Darwin ARM64)',
        cmd: 'IPHONEOS_DEPLOYMENT_TARGET=15.0 ./configure --target=ios\nmake'
      }
    };

    document.getElementById('pg-code').value = EXAMPLES.threads;

    function loadExample(key, btn) {
      document.querySelectorAll('.pg-tab').forEach(b => b.classList.remove('active'));
      if (btn) btn.classList.add('active');
      document.getElementById('pg-code').value = EXAMPLES[key];
    }

    function selectPlatform(key, btn) {
      document.querySelectorAll('.tab-btn').forEach(b => b.classList.remove('active'));
      if (btn) btn.classList.add('active');
      document.getElementById('platform-title').textContent = PLATFORMS[key].title;
      document.getElementById('platform-cmd').textContent = PLATFORMS[key].cmd;
    }

    async function runOxygenPlayground() {
      const code = document.getElementById('pg-code').value;
      const outEl = document.getElementById('pg-out');
      outEl.textContent = 'Running...';

      const globals = new Map();
      const tasks = new Map();
      const channels = new Map();
      const waiters = new Map();
      const atomics = new Map();
      const pending = [];
      const output = [];

      function evalExpr(raw, local) {
        const expr = raw.trim();
        if (!expr) return '';
        if (expr.includes(' plus ')) {
          return expr.split(/\s+plus\s+/i).map(p => evalExpr(p, local)).join('');
        }
        if ((expr.startsWith('"') && expr.endsWith('"')) || (expr.startsWith("'") && expr.endsWith("'"))) {
          return expr.slice(1, -1);
        }
        if (/^-?\d+(\.\d+)?$/.test(expr)) return Number(expr);
        if (local.has(expr)) return local.get(expr);
        if (globals.has(expr)) return globals.get(expr);
        return expr;
      }

      function evalCond(cond, local) {
        const low = cond.toLowerCase();
        if (low.includes(' is greater than ')) {
          const idx = low.indexOf(' is greater than ');
          return Number(evalExpr(cond.slice(0, idx), local)) > Number(evalExpr(cond.slice(idx + 17), local));
        }
        if (low.includes(' is less than ')) {
          const idx = low.indexOf(' is less than ');
          return Number(evalExpr(cond.slice(0, idx), local)) < Number(evalExpr(cond.slice(idx + 14), local));
        }
        if (low.includes(' is equal to ')) {
          const idx = low.indexOf(' is equal to ');
          return String(evalExpr(cond.slice(0, idx), local)) === String(evalExpr(cond.slice(idx + 13), local));
        }
        return Boolean(evalExpr(cond, local));
      }

      async function execBlock(lines, local) {
        for (let i = 0; i < lines.length; i++) {
          const raw = lines[i].trim();
          if (!raw || raw.startsWith('#') || raw.startsWith('//')) continue;
          const low = raw.toLowerCase();

          if (low.startsWith('define task ')) {
            const rest = raw.slice(12).trim();
            const wp = rest.toLowerCase().indexOf(' with parameter ');
            const name = wp !== -1 ? rest.slice(0, wp).trim() : rest;
            const param = wp !== -1 ? rest.slice(wp + 16).trim() : null;
            const body = [];
            i++;
            while (i < lines.length && lines[i].trim().toLowerCase() !== 'end task') {
              body.push(lines[i]);
              i++;
            }
            tasks.set(name, { param, body });
          } else if (low.startsWith('if ') && low.endsWith(' then')) {
            const cond = raw.slice(3, -5);
            const thenB = [], elseB = [];
            let inElse = false;
            i++;
            while (i < lines.length && lines[i].trim().toLowerCase() !== 'end if') {
              if (lines[i].trim().toLowerCase() === 'otherwise') { inElse = true; i++; continue; }
              (inElse ? elseB : thenB).push(lines[i]);
              i++;
            }
            await execBlock(evalCond(cond, local) ? thenB : elseB, local);
          } else if (low.startsWith('repeat ') && low.endsWith(' times')) {
            const count = Number(evalExpr(raw.slice(7, -6), local)) || 0;
            const body = [];
            i++;
            while (i < lines.length && lines[i].trim().toLowerCase() !== 'end repeat') {
              body.push(lines[i]);
              i++;
            }
            for (let k = 1; k <= count; k++) {
              local.set('iteration', k);
              await execBlock(body, local);
            }
          } else if (low.startsWith('print text ')) {
            output.push(String(evalExpr(raw.slice(11), local)));
          } else if (low.startsWith('print ')) {
            output.push(String(evalExpr(raw.slice(6), local)));
          } else if (low.startsWith('set ') && low.includes(' to ')) {
            const rest = raw.replace(/^set\s+(number|text|boolean|list)?\s*/i, '');
            const idx = rest.toLowerCase().indexOf(' to ');
            const name = rest.slice(0, idx).trim();
            const val = evalExpr(rest.slice(idx + 4), local);
            if (local.has(name)) local.set(name, val); else globals.set(name, val);
          } else if (low.startsWith('create channel ')) {
            const c = raw.slice(15).trim();
            channels.set(c, []);
            waiters.set(c, []);
          } else if (low.startsWith('send ') && low.includes(' to channel ')) {
            const idx = low.indexOf(' to channel ');
            const msg = String(evalExpr(raw.slice(5, idx).replace(/^text\s+/i, ''), local));
            const ch = raw.slice(idx + 12).trim();
            const w = waiters.get(ch) || [];
            if (w.length > 0) w.shift()(msg);
            else (channels.get(ch) || []).push(msg);
          } else if (low.startsWith('receive message from channel ')) {
            const rest = raw.slice(29);
            const idx = rest.toLowerCase().indexOf(' into ');
            const ch = rest.slice(0, idx).trim();
            const v = rest.slice(idx + 6).trim();
            const q = channels.get(ch) || [];
            const msg = q.length > 0 ? q.shift() : await new Promise(r => (waiters.get(ch) || []).push(r));
            local.set(v, msg);
          } else if (low.startsWith('create atomic counter ')) {
            const rest = raw.slice(22);
            const idx = rest.toLowerCase().indexOf(' with value ');
            const name = idx !== -1 ? rest.slice(0, idx).trim() : rest.trim();
            const val = idx !== -1 ? Number(evalExpr(rest.slice(idx + 12), local)) : 0;
            atomics.set(name, val);
          } else if (low.startsWith('increment atomic counter ')) {
            const name = raw.slice(25).trim();
            atomics.set(name, (atomics.get(name) || 0) + 1);
          } else if (low.startsWith('read atomic counter ') && low.includes(' into ')) {
            const idx = low.indexOf(' into ');
            const name = raw.slice(20, idx).trim();
            const v = raw.slice(idx + 6).trim();
            globals.set(v, atomics.get(name) || 0);
          } else if (low.startsWith('sleep for ')) {
            const ms = Number(evalExpr(raw.slice(10).replace(/milliseconds?/i, ''), local)) || 0;
            await new Promise(r => setTimeout(r, Math.min(500, ms)));
          } else if (low.startsWith('run task ')) {
            const bgIdx = low.indexOf(' in background');
            const part = bgIdx !== -1 ? raw.slice(9, bgIdx).trim() : raw.slice(9).trim();
            const wp = part.toLowerCase().indexOf(' with ');
            const name = wp !== -1 ? part.slice(0, wp).trim() : part;
            const arg = wp !== -1 ? evalExpr(part.slice(wp + 6), local) : '';
            const def = tasks.get(name);
            if (def) {
              const runFn = async () => {
                const scope = new Map();
                if (def.param) scope.set(def.param, arg);
                await execBlock(def.body, scope);
              };
              if (bgIdx !== -1) pending.push(runFn());
              else await runFn();
            }
          } else if (low.startsWith('wait for all background tasks')) {
            await Promise.all(pending.splice(0));
          }
        }
      }

      await execBlock(code.split(/\r?\n/), new Map());
      await Promise.all(pending);
      outEl.textContent = output.length > 0 ? output.join('\n') : '(Program finished with no output)';
    }
  </script>
</body>
</html>
