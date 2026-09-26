#pragma once
#include <stddef.h>

void *ft_user_alloc(size_t n);
void *ft_user_realloc(void *ptr, size_t size_old, size_t size_new);
void ft_user_free(void *ptr, size_t n);
void ft_user_log(const char *fmt, ...);
