# Oxygen Programming Language Manual

Creator: Fructose  
Source Repository: https://github.com/fructose15/oxygen  
File Extension: `.o2`

## 1. Philosophy & English-Prose Grammar

Oxygen eliminates cryptic punctuation in favor of readable English sentences. Every statement starts with an action verb (`print`, `set`, `add`, `subtract`, `multiply`, `divide`, `define`, `run`, `wait`, `create`, `lock`, `unlock`, `send`, `receive`, `if`, `repeat`, `while`, `for each`, `use`).

### Variables and Expressions

- `set number count to 10`
- `set text greeting to "Hello Oxygen"`
- `set boolean ready to true`
- `set list items to ["oxygen", "nitrogen", "argon"]`
- `add 5 to count`
- `subtract 2 from count`
- `multiply count by 3`
- `divide count by 2`
- `increment count`
- `decrement count`

### Output

- `print text "Hello world"`
- `print value of count`
- `print text "Count is " plus count`

### Control Flow

```text
if count is greater than 10 then
    print text "Threshold exceeded"
otherwise
    print text "Within safe range"
end if
```

Loops:

```text
repeat 4 times
    print text "Cycle #" plus iteration
end repeat

while count is less than 20 do
    increment count
end while

for each element in items do
    print text "Element: " plus element
end for
```

## 2. Multithreading & Concurrency

Oxygen embeds POSIX threads (`pthread`) on Linux, macOS, Android, and iOS, and Win32 threads (`CreateThread` / `CRITICAL_SECTION`) via `config.h` on Windows.

### Background Tasks

```text
define task process_chunk with parameter label
    print text "Processing " plus label
    sleep for 50 milliseconds
end task

run task process_chunk with "Segment A" in background as worker_a
run task process_chunk with "Segment B" in background as worker_b
wait for task worker_a
wait for task worker_b
```

Or wait for every active thread simultaneously:

```text
wait for all background tasks to finish
```

### Mutexes, Channels & Atomic Counters

```text
create mutex shared_lock
lock mutex shared_lock
print text "Inside critical section"
unlock mutex shared_lock

create channel job_queue
send text "compute_hash" to channel job_queue
receive message from channel job_queue into next_job

create atomic counter hits with value 0
increment atomic counter hits
read atomic counter hits into current_hits
```

## 3. Native C/C++ Embedding (`#include <oxygen.h>`)

Native C and C++ extensions include `src/oxygen.h` (installed as `<oxygen.h>`):

```c
#include <oxygen.h>

static int sample_command(int argc, const char **argv, char *out_buf, size_t out_cap) {
    (void)argc; (void)argv;
    snprintf(out_buf, out_cap, "Hello from native Oxygen module");
    return 0;
}

OXYGEN_REGISTER_MODULE("sample", "Fructose", 1, sample_command)
```

## 4. Complete CLI Reference (`oxygen`)

- `oxygen --command <command>` — Execute a single Oxygen command directly
- `oxygen --install-command <command_name>` — Download and install a command by name
- `oxygen --install-module <module_name>` — Download and install a module by name
- `oxygen --create-command <command_file> <command_name>` — Publish a command publicly
- `oxygen --create-module <binary_file> <module_name>` — Publish a module publicly
- `oxygen --delete-my-command <command_name>` — Delete a command you published publicly
- `oxygen --delete-my-module <module_name>` — Delete a module you published publicly
- `oxygen --delete-command <command_name>` — Delete a locally owned command
- `oxygen --delete-module <module_name>` — Delete a locally owned module
- `oxygen --ide` — Open the cross-platform Oxygen IDE window
- `oxygen --save-command <command_file> <command_name>` — Save a command into Oxygen
- `oxygen --update` — Update Oxygen to the latest release
- `oxygen --save-module <binary_file> <module_name>` — Save a module into Oxygen
- `oxygen --login` — Log in to your Oxygen account before publishing
- `oxygen --logout` — Log out of your Oxygen account
- `oxygen --help` — Display supported CLI options
- `oxygen --compile <script_path> -o <output_binary>` — Compile an `.o2` script into a native executable
- `oxygen --run <script_path>` — Execute an `.o2` script file
- `oxygen --readme` — Print the Oxygen README
- `oxygen --manual` — Print the Oxygen MANUAL
- `oxygen --license` — Print the Oxygen LICENSE
- `oxygen --agents` — Print the Oxygen agents.md
- `oxygen --codeowner` — Print the creator of Oxygen (`Fructose`)
- `oxygen --repo` — Direct to https://github.com/fructose15/oxygen

## 5. Standard 1 MB Command Library

Oxygen ships with a comprehensive catalog (`commands/oxygen_commands.o2cat`) exceeding 1 MB of English-syntax commands spanning 24 domains including Multithreading, Channels, Atomics, Math, Linear Algebra, Statistics, Text Processing, Collections, File System, Streams, Networking, Cryptography, Time & Scheduling, Compression, JSON/Data Serialization, Diagnostics, and Cross-Platform UI.
