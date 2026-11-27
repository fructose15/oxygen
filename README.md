# Oxygen Programming Language

Oxygen is a general-purpose, multithreaded programming language designed so that every instruction reads like clear, natural English prose. Files use the `.o2` extension.

- **Creator & Codeowner**: Fructose
- **Repository**: https://github.com/fructose15/oxygen
- **Implementation**: C and C++ (`src/oxygen.h`, `src/runtime.c`, `src/interpreter.cpp`, `src/compiler.cpp`, `src/main.cpp`)
- **Supported Platforms**: Linux, macOS, Windows, Android, and iOS

## Quick Start

### Linux, macOS, Android (Termux / NDK), and iOS

```sh
chmod +x configure
./configure
make
sudo make install
```

`make install` installs the `oxygen` binary, copies `src/oxygen.h` to `include/oxygen.h` so C/C++ extensions can `#include <oxygen.h>`, and installs the 1 MB standard command catalog.

### Windows

Windows builds use `config.h` directly (with MSVC `cl.exe` or MinGW `g++.exe`) without running `./configure`:

```bat
cl /EHsc /O2 /Isrc /Fe:oxygen.exe src\runtime.c src\interpreter.cpp src\compiler.cpp src\ide_server.cpp src\main.cpp ws2_32.lib
```

## Example Program (`hello.o2`)

```text
use module threading

print text "Starting Oxygen concurrent workers..."

create atomic counter completed_jobs with value 0

define task worker with parameter worker_name
    print text "Worker started: " plus worker_name
    sleep for 40 milliseconds
    increment atomic counter completed_jobs
    print text "Worker finished: " plus worker_name
end task

run task worker with "Alpha" in background as t1
run task worker with "Beta" in background as t2

wait for all background tasks to finish

read atomic counter completed_jobs into total
print text "Total jobs finished: " plus total
```

Run it with:

```sh
oxygen --run hello.o2
```

Or compile it to a standalone native binary:

```sh
oxygen --compile hello.o2 -o hello
./hello
```
