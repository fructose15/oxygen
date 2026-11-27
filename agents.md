# Oxygen Agents Guide (`agents.md`)

This document outlines conventions for automated tools, contributors, and coding agents working inside the Oxygen programming language codebase.

- **Codeowner**: Fructose (`https://github.com/fructose15/oxygen`)
- **Language Philosophy**: All Oxygen commands must read like clear, grammatical English sentences (`print text "..."`, `wait for all background tasks to finish`).
- **No Version Disclosures**: Do not expose version numbers in CLI help, IDE banners, or language outputs.
- **Cross-Platform Parity**:
  - POSIX targets (Linux, macOS, Android, iOS) build via `./configure` -> `Makefile`.
  - Windows targets build via `config.h` (`_WIN32` / MSVC / MinGW).
- **Header Contract**: Keep `src/oxygen.h` self-contained and C/C++ compatible (`extern "C"` guards) so downloaded/installed extensions can `#include <oxygen.h>`.
- **Thread Safety**: Any new runtime primitive added to `src/runtime.c` or `src/interpreter.cpp` must protect shared state with mutexes or atomic operations and pass multithreaded stress tests.
