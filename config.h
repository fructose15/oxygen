#ifndef OXYGEN_CONFIG_H
#define OXYGEN_CONFIG_H

/*
 * Oxygen Windows Build Configuration (config.h)
 * Creator: Fructose
 *
 * Used when building Oxygen and installing commands on Windows
 * (MSVC, Clang-CL, or MinGW-w64) without running the POSIX ./configure script.
 */

#if defined(_WIN32) || defined(_WIN64)
#  define OXYGEN_PLATFORM_WINDOWS 1
#  define OXYGEN_PLATFORM_NAME "Windows"
#  ifndef WIN32_LEAN_AND_MEAN
#    define WIN32_LEAN_AND_MEAN
#  endif
#  include <winsock2.h>
#  include <ws2tcpip.h>
#  include <windows.h>
#  include <direct.h>
#  include <io.h>
#  include <shellapi.h>

#  define OXYGEN_PATH_SEP "\\"
#  define OXYGEN_PATH_MAX_LEN 4096
#  define OXYGEN_HOME_ENV "USERPROFILE"
#  define OXYGEN_DATA_SUBDIR "\\.oxygen"
#  define OXYGEN_DEFAULT_COMPILER "cl.exe"
#  define OXYGEN_FALLBACK_COMPILER "g++.exe"
#  define OXYGEN_BINARY_EXT ".exe"
#  define OXYGEN_MODULE_EXT ".dll"

/* Win32 synchronization & threading mapping */
typedef HANDLE oxygen_native_thread_t;
typedef CRITICAL_SECTION oxygen_native_mutex_t;
typedef volatile LONG64 oxygen_native_atomic_t;

#  define oxygen_native_sleep(ms) Sleep((DWORD)(ms))
#  define oxygen_native_mkdir(dir) _mkdir(dir)
#  define oxygen_strcasecmp _stricmp
#  define oxygen_strncasecmp _strnicmp

/* Windows command registration & compiler flags */
#  define OXYGEN_WIN_ENABLE_UTF8_CONSOLE 1
#  define OXYGEN_WIN_THREAD_POOL_MAX 256
#  define OXYGEN_CATALOG_RELATIVE_PATH "commands\\oxygen_commands.o2cat"
#  define OXYGEN_INCLUDE_DIR "include"
#  define OXYGEN_HEADER_TARGET "include\\oxygen.h"

#  ifdef _MSC_VER
#    pragma comment(lib, "shell32.lib")
#    pragma comment(lib, "user32.lib")
#    pragma comment(lib, "ws2_32.lib")
#  endif
#endif

#endif /* OXYGEN_CONFIG_H */
