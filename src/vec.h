#pragma once
#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>
#include "ft.h"

struct fti_vec {
    // all of these are private, and the user shouldn't touch them :)

    size_t obj_size;
    void *userctx;
    void *(*obj_init)(void *userctx, void *ptr);
    void (*obj_destroy)(void *userctx, void *ptr);

    size_t size;
    size_t capacity;
    void *data;
};

// these allocate the whole vector on the heap
struct fti_vec *fti_vec_create(size_t obj_size, void *userctx, void *(*obj_init)(void *userctx, void *ptr), void (*obj_destroy)(void *userctx, void *ptr));
void fti_vec_destroy(struct fti_vec *vec);

// these can be used for stack-allocated vectors
void fti_vec_init(struct fti_vec *vec, size_t obj_size, void *userctx, void *(*obj_init)(void *userctx, void *ptr), void (*obj_destroy)(void *userctx, void *ptr));
void fti_vec_deinit(struct fti_vec *vec);

// things that deal with size & capacity directly
inline size_t fti_vec_size(struct fti_vec *vec) {
    return vec->size;
}
inline size_t fti_vec_capacity(struct fti_vec *vec) {
    return vec->capacity;
}
void fti_vec_reserve(struct fti_vec *vec, size_t min_capacity);
void fti_vec_shrink_to_fit(struct fti_vec *vec);
bool fti_vec_empty(struct fti_vec *vec) {
    return vec->size == 0;
}

// (mass) erase operations on the vector
void fti_vec_erase_range(struct fti_vec *vec, size_t idx_first, size_t idx_last);
inline void fti_vec_erase(struct fti_vec *vec, size_t idx) {
    fti_vec_erase_range(vec, idx, idx);
}
void fti_vec_clear(struct fti_vec *vec);

// element accessors
inline void* fti_vec_idx(struct fti_vec *vec, size_t idx) {
    return &((uint8_t *)vec->data)[idx * vec->obj_size];
}

// adding & removing elements
void* fti_vec_push_back(struct fti_vec *vec);
void fti_vec_pop_back(struct fti_vec *vec);
