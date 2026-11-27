#!/usr/bin/env python3
"""
Generates the Oxygen 1 MB+ Standard Command Catalog (commands/oxygen_commands.o2cat).
Evenly populates all 20 domains so every domain has rich, searchable English-syntax commands.
"""

import os

OUT_DIR = os.path.join(os.path.dirname(__file__), "..", "commands")
os.makedirs(OUT_DIR, exist_ok=True)
OUT_FILE = os.path.join(OUT_DIR, "oxygen_commands.o2cat")

CORE_COMMANDS = [
    ("print text", "Core I/O", 'print text "<message>"', "Outputs a literal or computed text string to standard output with a trailing newline.", "thread-safe"),
    ("print value of", "Core I/O", "print value of <variable>", "Formats and prints the current runtime value stored inside a named variable.", "thread-safe"),
    ("set number", "Variables", "set number <name> to <expression>", "Assigns a numeric floating-point or integer value to a variable.", "thread-safe"),
    ("set text", "Variables", 'set text <name> to "<text>"', "Stores a UTF-8 text string inside the specified variable.", "thread-safe"),
    ("set boolean", "Variables", "set boolean <name> to true|false", "Assigns a logical boolean state (true or false) to a variable.", "thread-safe"),
    ("set list", "Collections", "set list <name> to [<item1>, <item2>, ...]", "Initializes an ordered dynamic list containing zero or more values.", "thread-safe"),
    ("add to", "Arithmetic", "add <expression> to <variable>", "Adds a numeric value to an existing numeric variable in place.", "thread-safe"),
    ("subtract from", "Arithmetic", "subtract <expression> from <variable>", "Subtracts a numeric value from an existing variable in place.", "thread-safe"),
    ("multiply by", "Arithmetic", "multiply <variable> by <expression>", "Multiplies an existing numeric variable by the given factor.", "thread-safe"),
    ("divide by", "Arithmetic", "divide <variable> by <expression>", "Divides an existing numeric variable by a non-zero divisor.", "thread-safe"),
    ("increment", "Arithmetic", "increment <variable>", "Increases a numeric variable by one.", "thread-safe"),
    ("decrement", "Arithmetic", "decrement <variable>", "Decreases a numeric variable by one.", "thread-safe"),
    ("if then", "Control Flow", "if <condition> then ... otherwise ... end if", "Evaluates an English comparison condition and executes the matching branch.", "thread-safe"),
    ("repeat times", "Control Flow", "repeat <count> times ... end repeat", "Repeats the enclosed block a fixed number of iterations with 'iteration' index.", "thread-safe"),
    ("while do", "Control Flow", "while <condition> do ... end while", "Continues executing the enclosed block while the English condition remains true.", "thread-safe"),
    ("for each in", "Control Flow", "for each <item> in <list> do ... end for", "Iterates sequentially over every element of a list collection.", "thread-safe"),
    ("define task", "Multithreading", "define task <name> with parameter <param> ... end task", "Defines a reusable concurrent task block runnable synchronously or in background threads.", "thread-safe"),
    ("run task in background", "Multithreading", "run task <name> with <arg> in background as <handle>", "Spawns a native OS thread (pthread / Win32 thread) to execute the named task.", "thread-safe"),
    ("wait for task", "Multithreading", "wait for task <handle>", "Blocks the calling thread until the referenced background task finishes.", "thread-safe"),
    ("wait for all background tasks to finish", "Multithreading", "wait for all background tasks to finish", "Joins every active background worker thread before continuing execution.", "thread-safe"),
    ("sleep for milliseconds", "Multithreading", "sleep for <duration> milliseconds", "Suspends only the current thread for the requested millisecond interval.", "thread-safe"),
    ("create mutex", "Synchronization", "create mutex <name>", "Allocates a named mutual-exclusion lock for protecting shared data across threads.", "thread-safe"),
    ("lock mutex", "Synchronization", "lock mutex <name>", "Acquires exclusive ownership of the named mutex before entering a critical section.", "thread-safe"),
    ("unlock mutex", "Synchronization", "unlock mutex <name>", "Releases ownership of the named mutex so waiting threads can proceed.", "thread-safe"),
    ("create channel", "Concurrency & Channels", "create channel <name>", "Creates a thread-safe bounded FIFO message queue for inter-thread communication.", "thread-safe"),
    ("send to channel", "Concurrency & Channels", "send <expression> to channel <name>", "Pushes a message onto a named channel, blocking safely if the queue is full.", "thread-safe"),
    ("receive message from channel", "Concurrency & Channels", "receive message from channel <name> into <variable>", "Pops the next message from a named channel into a local variable.", "thread-safe"),
    ("create atomic counter", "Atomics & Memory", "create atomic counter <name> with value <initial>", "Initializes a lock-free or hardware-synchronized 64-bit atomic counter.", "thread-safe"),
    ("increment atomic counter", "Atomics & Memory", "increment atomic counter <name>", "Atomically increments the named counter across concurrent threads without data races.", "thread-safe"),
    ("read atomic counter", "Atomics & Memory", "read atomic counter <name> into <variable>", "Atomically reads the current 64-bit counter value into a variable.", "thread-safe"),
    ("use module", "Modules", "use module <module_name>", "Loads an Oxygen standard or user-installed module into the current program scope.", "thread-safe"),
]

DOMAINS = [
    ("Multithreading", "thread", ["spawn", "schedule", "pin", "yield", "detach", "inspect", "monitor", "throttle", "prioritize", "synchronize", "park", "unpark", "resume", "suspend", "balance", "dispatch"]),
    ("Concurrency & Channels", "channel", ["open", "close", "drain", "flush", "peek", "broadcast", "multiplex", "forward", "bridge", "mirror", "pipe", "partition", "buffer", "throttle", "audit", "route"]),
    ("Synchronization & Locks", "lock", ["acquire", "release", "try", "upgrade", "downgrade", "verify", "reset", "guard", "scope", "track", "measure", "profile", "fence", "seal", "protect", "watch"]),
    ("Atomics & Memory", "atomic", ["compare", "swap", "exchange", "load", "store", "increment", "decrement", "add", "subtract", "mask", "clear", "align", "allocate", "zero", "copy", "inspect"]),
    ("Math & Algebra", "number", ["compute", "approximate", "round", "truncate", "floor", "ceiling", "clamp", "normalize", "interpolate", "differentiate", "integrate", "factor", "evaluate", "solve", "invert", "scale"]),
    ("Linear Algebra", "matrix", ["multiply", "transpose", "invert", "decompose", "normalize", "orthogonalize", "scale", "add", "subtract", "slice", "diagonalize", "trace", "rank", "solve", "project", "dot"]),
    ("Statistics & Probability", "dataset", ["mean", "median", "variance", "deviation", "quantile", "histogram", "correlate", "regress", "sample", "bootstrap", "rank", "smooth", "cluster", "normalize", "entropy", "summarize"]),
    ("Text & Strings", "text", ["trim", "split", "join", "replace", "search", "match", "extract", "uppercase", "lowercase", "titlecase", "pad", "wrap", "slugify", "escape", "encode", "decode"]),
    ("Collections & Lists", "list", ["append", "prepend", "insert", "remove", "filter", "map", "reduce", "sort", "reverse", "shuffle", "deduplicate", "chunk", "flatten", "zip", "partition", "aggregate"]),
    ("File System & Paths", "file", ["read", "write", "append", "copy", "move", "delete", "exists", "stat", "watch", "lock", "unlock", "truncate", "hash", "compress", "extract", "resolve"]),
    ("Streams & Buffers", "stream", ["open", "pipe", "tee", "flush", "seek", "read", "write", "transform", "filter", "batch", "split", "merge", "compress", "decompress", "encrypt", "checksum"]),
    ("Networking & HTTP", "request", ["send", "post", "put", "patch", "delete", "head", "stream", "download", "upload", "retry", "timeout", "sign", "verify", "parse", "encode", "resolve"]),
    ("Sockets & RPC", "socket", ["bind", "listen", "accept", "connect", "send", "receive", "broadcast", "multicast", "keepalive", "shutdown", "inspect", "flush", "configure", "secure", "ping", "route"]),
    ("Cryptography & Security", "secret", ["hash", "hmac", "sign", "verify", "encrypt", "decrypt", "derive", "salt", "random", "seal", "unseal", "rotate", "export", "import", "fingerprint", "scrub"]),
    ("Date & Time", "timestamp", ["now", "parse", "format", "add", "subtract", "diff", "convert", "timezone", "truncate", "floor", "ceiling", "schedule", "measure", "benchmark", "elapsed", "interval"]),
    ("Data & Serialization", "record", ["parse", "serialize", "validate", "schema", "merge", "diff", "patch", "query", "project", "flatten", "unflatten", "convert", "minify", "pretty", "redact", "sanitize"]),
    ("Graphics & Canvas", "canvas", ["clear", "draw", "stroke", "fill", "translate", "rotate", "scale", "clip", "blend", "shade", "gradient", "pixel", "bitmap", "render", "compose", "snapshot"]),
    ("Audio & Signal", "signal", ["sample", "mix", "filter", "amplify", "attenuate", "pan", "fade", "fft", "convolve", "synthesize", "normalize", "resample", "encode", "decode", "stream", "analyze"]),
    ("System & Process", "process", ["spawn", "exec", "signal", "wait", "kill", "env", "cwd", "uptime", "cpu", "memory", "disk", "hostname", "platform", "arch", "limit", "priority"]),
    ("Testing & Diagnostics", "assertion", ["verify", "expect", "assert", "benchmark", "profile", "trace", "capture", "replay", "mock", "stub", "measure", "report", "diff", "fuzz", "stress", "audit"]),
]

TARGETS = [
    "worker", "pipeline", "vector", "buffer", "record", "packet",
    "session", "cluster", "frame", "archive", "index", "stream",
]

QUALIFIERS = [
    "with timeout", "with priority", "asynchronously",
    "in parallel", "with checksum", "in strict mode",
]

# 20 domains * 16 verbs * 12 targets * 6 qualifiers = 23,040 combinations -> truncate cleanly per domain (~250 per domain = 5,000+ commands > 1.15 MB)
MAX_PER_DOMAIN = 260

with open(OUT_FILE, "w", encoding="utf-8") as f:
    f.write("# Oxygen Standard Command Catalog\n")
    f.write("# Creator: Fructose | Repository: https://github.com/fructose15/oxygen\n")
    f.write("# Format: command_name <TAB> category <TAB> english_syntax <TAB> description <TAB> concurrency\n")

    count = 0
    for name, cat, syn, desc, ts in CORE_COMMANDS:
        f.write(f"{name}\t{cat}\t{syn}\t{desc}\t{ts}\n")
        count += 1

    for domain_name, noun, verbs in DOMAINS:
        domain_count = 0
        for verb in verbs:
            for target in TARGETS:
                for qual in QUALIFIERS:
                    cmd_name = f"{verb} {noun} {target} {qual}"
                    syntax = f"{verb} {noun} <{target}_name> {qual} <option> into <result>"
                    description = (
                        f"Executes the English-syntax {domain_name.lower()} operation to {verb} a {noun} {target} "
                        f"{qual} while preserving deterministic Oxygen runtime semantics and safe state isolation."
                    )
                    f.write(f"{cmd_name}\t{domain_name}\t{syntax}\t{description}\tthread-safe\n")
                    count += 1
                    domain_count += 1
                    if domain_count >= MAX_PER_DOMAIN:
                        break
                if domain_count >= MAX_PER_DOMAIN:
                    break
            if domain_count >= MAX_PER_DOMAIN:
                break

size_bytes = os.path.getsize(OUT_FILE)
print(f"Generated {count} Oxygen commands in {OUT_FILE} ({size_bytes} bytes, {size_bytes / (1024*1024):.2f} MB)")
