#pragma once
#include <foxticulum/foxticulum.h>
#include <foxticulum/interface.h>
#include <foxticulum/user.h>

#define FT_LOG_ERR 1
#define FT_LOG_WARN 2
#define FT_LOG_INFO 3
#define FT_LOG_DEBUG 4
#define FT_LOG(level, ...) ft_user_log(__VA_ARGS__)

#define FT_LOG_BYTES(level, prefix, suffix, bytefmt, bytes, len) do { \
    if (prefix) { \
        FT_LOG(level, prefix); \
    } \
    for (size_t i = 0; i < len; i++) { \
        FT_LOG(level, bytefmt, ((uint8_t *)(bytes))[i]); \
    } \
    if (suffix) { \
        FT_LOG(level, suffix); \
    } \
} while (0)

struct foxticulum_handle {
    struct ft_config config;
};

#define HANDLE(x) ((struct foxticulum_handle *)(x))

// C standard functions that really should be implemented :)
void *memcpy(void *restrict dest, const void *restrict src, size_t count);
void* memset(void* dest, int ch, size_t count);
void* memmove(void* dest, const void* src, size_t count);
