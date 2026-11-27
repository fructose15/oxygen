#include "oxygen.h"

#if !defined(_WIN32) && defined(__has_include)
#  if __has_include("platform_config.h")
#    include "platform_config.h"
#  endif
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef _WIN32
#  include <process.h>
#else
#  include <pthread.h>
#  include <unistd.h>
#  include <strings.h>
#endif

#define OXYGEN_MAX_TRACKED_THREADS 512

struct oxygen_thread_handle {
    char task_name[128];
    oxygen_task_fn fn;
    void *arg;
    int completed;
#ifdef _WIN32
    HANDLE handle;
#else
    pthread_t thread;
#endif
};

struct oxygen_mutex_handle {
    char name[128];
#ifdef _WIN32
    CRITICAL_SECTION cs;
#else
    pthread_mutex_t mtx;
#endif
};

struct oxygen_channel_handle {
    char name[128];
    char **queue;
    size_t capacity;
    size_t head;
    size_t tail;
    size_t count;
#ifdef _WIN32
    CRITICAL_SECTION lock;
    CONDITION_VARIABLE not_empty;
    CONDITION_VARIABLE not_full;
#else
    pthread_mutex_t lock;
    pthread_cond_t not_empty;
    pthread_cond_t not_full;
#endif
};

struct oxygen_atomic_counter {
    char name[128];
#ifdef _WIN32
    volatile LONG64 value;
#else
    int64_t value;
    pthread_mutex_t lock;
#endif
};

static oxygen_thread_handle_t *g_threads[OXYGEN_MAX_TRACKED_THREADS];
static size_t g_thread_count = 0;

#ifdef _WIN32
static CRITICAL_SECTION g_registry_lock;
#else
static pthread_mutex_t g_registry_lock = PTHREAD_MUTEX_INITIALIZER;
#endif
static int g_runtime_initialized = 0;

int oxygen_runtime_init(void) {
    if (g_runtime_initialized) {
        return 0;
    }
#ifdef _WIN32
    InitializeCriticalSection(&g_registry_lock);
#  ifdef OXYGEN_WIN_ENABLE_UTF8_CONSOLE
    SetConsoleOutputCP(65001);
#  endif
#endif
    memset(g_threads, 0, sizeof(g_threads));
    g_thread_count = 0;
    g_runtime_initialized = 1;
    return 0;
}

void oxygen_runtime_shutdown(void) {
    if (!g_runtime_initialized) {
        return;
    }
    oxygen_thread_join_all();
#ifdef _WIN32
    DeleteCriticalSection(&g_registry_lock);
#endif
    g_runtime_initialized = 0;
}

#ifdef _WIN32
static unsigned __stdcall oxygen_thread_trampoline(void *raw) {
    oxygen_thread_handle_t *h = (oxygen_thread_handle_t *)raw;
    if (h && h->fn) {
        h->fn(h->arg);
    }
    if (h) {
        h->completed = 1;
    }
    return 0;
}
#else
static void *oxygen_thread_trampoline(void *raw) {
    oxygen_thread_handle_t *h = (oxygen_thread_handle_t *)raw;
    if (h && h->fn) {
        h->fn(h->arg);
    }
    if (h) {
        h->completed = 1;
    }
    return NULL;
}
#endif

oxygen_thread_handle_t *oxygen_thread_spawn(const char *task_name, oxygen_task_fn fn, void *arg) {
    oxygen_thread_handle_t *h = (oxygen_thread_handle_t *)calloc(1, sizeof(oxygen_thread_handle_t));
    if (!h) {
        return NULL;
    }
    snprintf(h->task_name, sizeof(h->task_name), "%s", task_name ? task_name : "task");
    h->fn = fn;
    h->arg = arg;
    h->completed = 0;

#ifdef _WIN32
    EnterCriticalSection(&g_registry_lock);
#else
    pthread_mutex_lock(&g_registry_lock);
#endif
    if (g_thread_count < OXYGEN_MAX_TRACKED_THREADS) {
        g_threads[g_thread_count++] = h;
    }
#ifdef _WIN32
    LeaveCriticalSection(&g_registry_lock);
    uintptr_t tid = _beginthreadex(NULL, 0, oxygen_thread_trampoline, h, 0, NULL);
    h->handle = (HANDLE)tid;
    if (!h->handle) {
        free(h);
        return NULL;
    }
#else
    pthread_mutex_unlock(&g_registry_lock);
    if (pthread_create(&h->thread, NULL, oxygen_thread_trampoline, h) != 0) {
        free(h);
        return NULL;
    }
#endif
    return h;
}

int oxygen_thread_join(oxygen_thread_handle_t *handle) {
    if (!handle) {
        return -1;
    }
#ifdef _WIN32
    if (handle->handle) {
        WaitForSingleObject(handle->handle, INFINITE);
        CloseHandle(handle->handle);
        handle->handle = NULL;
    }
#else
    if (!handle->completed || handle->thread) {
        pthread_join(handle->thread, NULL);
        handle->thread = 0;
    }
#endif
    handle->completed = 1;
    return 0;
}

int oxygen_thread_join_all(void) {
    size_t i = 0;
    for (i = 0; i < g_thread_count; ++i) {
        if (g_threads[i]) {
            oxygen_thread_join(g_threads[i]);
            free(g_threads[i]);
            g_threads[i] = NULL;
        }
    }
    g_thread_count = 0;
    return 0;
}

int oxygen_active_thread_count(void) {
    int count = 0;
    size_t i = 0;
#ifdef _WIN32
    EnterCriticalSection(&g_registry_lock);
#else
    pthread_mutex_lock(&g_registry_lock);
#endif
    for (i = 0; i < g_thread_count; ++i) {
        if (g_threads[i] && !g_threads[i]->completed) {
            count++;
        }
    }
#ifdef _WIN32
    LeaveCriticalSection(&g_registry_lock);
#else
    pthread_mutex_unlock(&g_registry_lock);
#endif
    return count;
}

void oxygen_sleep_ms(uint32_t milliseconds) {
#ifdef _WIN32
    oxygen_native_sleep(milliseconds);
#else
    usleep((useconds_t)milliseconds * 1000);
#endif
}

oxygen_mutex_handle_t *oxygen_mutex_create(const char *name) {
    oxygen_mutex_handle_t *m = (oxygen_mutex_handle_t *)calloc(1, sizeof(oxygen_mutex_handle_t));
    if (!m) {
        return NULL;
    }
    snprintf(m->name, sizeof(m->name), "%s", name ? name : "mutex");
#ifdef _WIN32
    InitializeCriticalSection(&m->cs);
#else
    pthread_mutex_init(&m->mtx, NULL);
#endif
    return m;
}

int oxygen_mutex_lock(oxygen_mutex_handle_t *mutex) {
    if (!mutex) return -1;
#ifdef _WIN32
    EnterCriticalSection(&mutex->cs);
    return 0;
#else
    return pthread_mutex_lock(&mutex->mtx);
#endif
}

int oxygen_mutex_unlock(oxygen_mutex_handle_t *mutex) {
    if (!mutex) return -1;
#ifdef _WIN32
    LeaveCriticalSection(&mutex->cs);
    return 0;
#else
    return pthread_mutex_unlock(&mutex->mtx);
#endif
}

void oxygen_mutex_destroy(oxygen_mutex_handle_t *mutex) {
    if (!mutex) return;
#ifdef _WIN32
    DeleteCriticalSection(&mutex->cs);
#else
    pthread_mutex_destroy(&mutex->mtx);
#endif
    free(mutex);
}

oxygen_channel_handle_t *oxygen_channel_create(const char *name, size_t capacity) {
    oxygen_channel_handle_t *ch = (oxygen_channel_handle_t *)calloc(1, sizeof(oxygen_channel_handle_t));
    if (!ch) return NULL;
    if (capacity == 0) capacity = 64;
    snprintf(ch->name, sizeof(ch->name), "%s", name ? name : "channel");
    ch->capacity = capacity;
    ch->queue = (char **)calloc(capacity, sizeof(char *));
    if (!ch->queue) {
        free(ch);
        return NULL;
    }
#ifdef _WIN32
    InitializeCriticalSection(&ch->lock);
    InitializeConditionVariable(&ch->not_empty);
    InitializeConditionVariable(&ch->not_full);
#else
    pthread_mutex_init(&ch->lock, NULL);
    pthread_cond_init(&ch->not_empty, NULL);
    pthread_cond_init(&ch->not_full, NULL);
#endif
    return ch;
}

int oxygen_channel_send(oxygen_channel_handle_t *ch, const char *message) {
    char *copy = NULL;
    if (!ch || !message) return -1;
    copy = (char *)malloc(strlen(message) + 1);
    if (!copy) return -1;
    strcpy(copy, message);

#ifdef _WIN32
    EnterCriticalSection(&ch->lock);
    while (ch->count == ch->capacity) {
        SleepConditionVariableCS(&ch->not_full, &ch->lock, INFINITE);
    }
    ch->queue[ch->tail] = copy;
    ch->tail = (ch->tail + 1) % ch->capacity;
    ch->count++;
    WakeConditionVariable(&ch->not_empty);
    LeaveCriticalSection(&ch->lock);
#else
    pthread_mutex_lock(&ch->lock);
    while (ch->count == ch->capacity) {
        pthread_cond_wait(&ch->not_full, &ch->lock);
    }
    ch->queue[ch->tail] = copy;
    ch->tail = (ch->tail + 1) % ch->capacity;
    ch->count++;
    pthread_cond_signal(&ch->not_empty);
    pthread_mutex_unlock(&ch->lock);
#endif
    return 0;
}

int oxygen_channel_receive(oxygen_channel_handle_t *ch, char *out_buf, size_t out_cap) {
    char *msg = NULL;
    if (!ch || !out_buf || out_cap == 0) return -1;
#ifdef _WIN32
    EnterCriticalSection(&ch->lock);
    while (ch->count == 0) {
        SleepConditionVariableCS(&ch->not_empty, &ch->lock, INFINITE);
    }
    msg = ch->queue[ch->head];
    ch->queue[ch->head] = NULL;
    ch->head = (ch->head + 1) % ch->capacity;
    ch->count--;
    WakeConditionVariable(&ch->not_full);
    LeaveCriticalSection(&ch->lock);
#else
    pthread_mutex_lock(&ch->lock);
    while (ch->count == 0) {
        pthread_cond_wait(&ch->not_empty, &ch->lock);
    }
    msg = ch->queue[ch->head];
    ch->queue[ch->head] = NULL;
    ch->head = (ch->head + 1) % ch->capacity;
    ch->count--;
    pthread_cond_signal(&ch->not_full);
    pthread_mutex_unlock(&ch->lock);
#endif
    snprintf(out_buf, out_cap, "%s", msg ? msg : "");
    free(msg);
    return 0;
}

void oxygen_channel_destroy(oxygen_channel_handle_t *ch) {
    size_t i = 0;
    if (!ch) return;
    for (i = 0; i < ch->capacity; ++i) {
        free(ch->queue[i]);
    }
    free(ch->queue);
#ifdef _WIN32
    DeleteCriticalSection(&ch->lock);
#else
    pthread_mutex_destroy(&ch->lock);
    pthread_cond_destroy(&ch->not_empty);
    pthread_cond_destroy(&ch->not_full);
#endif
    free(ch);
}

oxygen_atomic_counter_t *oxygen_atomic_create(const char *name, int64_t initial_value) {
    oxygen_atomic_counter_t *counter = (oxygen_atomic_counter_t *)calloc(1, sizeof(oxygen_atomic_counter_t));
    if (!counter) return NULL;
    snprintf(counter->name, sizeof(counter->name), "%s", name ? name : "counter");
    counter->value = initial_value;
#ifndef _WIN32
    pthread_mutex_init(&counter->lock, NULL);
#endif
    return counter;
}

int64_t oxygen_atomic_increment(oxygen_atomic_counter_t *counter) {
    int64_t result = 0;
    if (!counter) return 0;
#ifdef _WIN32
    result = InterlockedIncrement64(&counter->value);
#else
    pthread_mutex_lock(&counter->lock);
    counter->value += 1;
    result = counter->value;
    pthread_mutex_unlock(&counter->lock);
#endif
    return result;
}

int64_t oxygen_atomic_decrement(oxygen_atomic_counter_t *counter) {
    int64_t result = 0;
    if (!counter) return 0;
#ifdef _WIN32
    result = InterlockedDecrement64(&counter->value);
#else
    pthread_mutex_lock(&counter->lock);
    counter->value -= 1;
    result = counter->value;
    pthread_mutex_unlock(&counter->lock);
#endif
    return result;
}

int64_t oxygen_atomic_read(oxygen_atomic_counter_t *counter) {
    int64_t result = 0;
    if (!counter) return 0;
#ifdef _WIN32
    result = InterlockedCompareExchange64(&counter->value, 0, 0);
#else
    pthread_mutex_lock(&counter->lock);
    result = counter->value;
    pthread_mutex_unlock(&counter->lock);
#endif
    return result;
}

const char *oxygen_detect_platform(void) {
#ifdef OXYGEN_PLATFORM_NAME
    return OXYGEN_PLATFORM_NAME;
#elif defined(_WIN32)
    return "Windows";
#elif defined(__APPLE__)
    return "macOS/iOS";
#elif defined(__ANDROID__)
    return "Android";
#else
    return "Linux";
#endif
}

int oxygen_open_browser_url(const char *url) {
    char cmd[1024];
    if (!url) return -1;
#ifdef _WIN32
    ShellExecuteA(NULL, "open", url, NULL, NULL, SW_SHOWNORMAL);
    return 0;
#elif defined(__APPLE__)
    snprintf(cmd, sizeof(cmd), "open \"%s\" >/dev/null 2>&1 &", url);
    return system(cmd);
#elif defined(__ANDROID__)
    snprintf(cmd, sizeof(cmd), "termux-open-url \"%s\" >/dev/null 2>&1 &", url);
    return system(cmd);
#else
    snprintf(cmd, sizeof(cmd), "xdg-open \"%s\" >/dev/null 2>&1 || echo \"Open in browser: %s\"", url, url);
    return system(cmd);
#endif
}
