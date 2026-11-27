#ifndef OXYGEN_H
#define OXYGEN_H

/*
 * Oxygen Programming Language Public Header (src/oxygen.h)
 * Creator & Codeowner: Fructose
 * Repository: https://github.com/fructose15/oxygen
 * File Extension: .o2
 *
 * When Oxygen is installed on Linux, macOS, Android, iOS, or Windows,
 * this header is exposed as <oxygen.h> for native C/C++ extensions,
 * compiled .o2 executables, and loadable binary modules.
 */

#ifdef _WIN32
#include "../config.h"
#endif

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#define OXYGEN_REPO_URL "https://github.com/fructose15/oxygen"
#define OXYGEN_CODEOWNER "Fructose"
#define OXYGEN_FILE_EXT ".o2"

typedef enum oxygen_value_kind {
    OXYGEN_VAL_NONE = 0,
    OXYGEN_VAL_NUMBER = 1,
    OXYGEN_VAL_TEXT = 2,
    OXYGEN_VAL_BOOLEAN = 3,
    OXYGEN_VAL_LIST = 4,
    OXYGEN_VAL_HANDLE = 5
} oxygen_value_kind_t;

typedef struct oxygen_value {
    oxygen_value_kind_t kind;
    double number_val;
    int bool_val;
    char text_val[1024];
} oxygen_value_t;

typedef struct oxygen_thread_handle oxygen_thread_handle_t;
typedef struct oxygen_mutex_handle oxygen_mutex_handle_t;
typedef struct oxygen_channel_handle oxygen_channel_handle_t;
typedef struct oxygen_atomic_counter oxygen_atomic_counter_t;

typedef void (*oxygen_task_fn)(void *arg);
typedef int (*oxygen_module_command_fn)(int argc, const char **argv, char *out_buf, size_t out_cap);

typedef struct oxygen_module_descriptor {
    const char *module_name;
    const char *author;
    int thread_safe;
    oxygen_module_command_fn dispatch;
} oxygen_module_descriptor_t;

/* Runtime lifecycle */
int oxygen_runtime_init(void);
void oxygen_runtime_shutdown(void);

/* Multithreading primitives */
oxygen_thread_handle_t *oxygen_thread_spawn(const char *task_name, oxygen_task_fn fn, void *arg);
int oxygen_thread_join(oxygen_thread_handle_t *handle);
int oxygen_thread_join_all(void);
int oxygen_active_thread_count(void);
void oxygen_sleep_ms(uint32_t milliseconds);

/* Synchronization: Mutexes */
oxygen_mutex_handle_t *oxygen_mutex_create(const char *name);
int oxygen_mutex_lock(oxygen_mutex_handle_t *mutex);
int oxygen_mutex_unlock(oxygen_mutex_handle_t *mutex);
void oxygen_mutex_destroy(oxygen_mutex_handle_t *mutex);

/* Concurrency: Message Channels */
oxygen_channel_handle_t *oxygen_channel_create(const char *name, size_t capacity);
int oxygen_channel_send(oxygen_channel_handle_t *channel, const char *message);
int oxygen_channel_receive(oxygen_channel_handle_t *channel, char *out_buf, size_t out_cap);
void oxygen_channel_destroy(oxygen_channel_handle_t *channel);

/* Concurrency: Atomic Counters */
oxygen_atomic_counter_t *oxygen_atomic_create(const char *name, int64_t initial_value);
int64_t oxygen_atomic_increment(oxygen_atomic_counter_t *counter);
int64_t oxygen_atomic_decrement(oxygen_atomic_counter_t *counter);
int64_t oxygen_atomic_read(oxygen_atomic_counter_t *counter);

/* Platform & System utilities */
const char *oxygen_detect_platform(void);
int oxygen_open_browser_url(const char *url);
int oxygen_run_embedded_script(const char *script_source);

/* Module export macro for C/C++ Oxygen binary modules */
#ifdef _WIN32
#define OXYGEN_EXPORT __declspec(dllexport)
#else
#define OXYGEN_EXPORT __attribute__((visibility("default")))
#endif

#define OXYGEN_REGISTER_MODULE(name, author_name, is_thread_safe, fn) \
    OXYGEN_EXPORT const oxygen_module_descriptor_t *oxygen_module_entry(void) { \
        static oxygen_module_descriptor_t desc = { name, author_name, is_thread_safe, fn }; \
        return &desc; \
    }

#ifdef __cplusplus
}
#endif

#endif /* OXYGEN_H */
