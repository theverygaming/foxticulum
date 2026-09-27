#include "vec.h"

static void vec_reallocate(struct fti_vec *vec, size_t capacity) {
    if (vec->capacity == capacity) {
        return;
    }
    if (capacity == 0) {
        ft_user_free(vec->data, vec->capacity * vec->obj_size);
        vec->data = NULL;
        vec->capacity = 0;
        return;
    }
    vec->data = ft_user_realloc(vec->data, vec->capacity * vec->obj_size, capacity * vec->obj_size);
    vec->capacity = capacity;
}

static void vec_ensure_size(struct fti_vec *vec, size_t size) {
    vec->size = size;
    if (vec->size > vec->capacity) {
        vec_reallocate(vec, vec->size);
    }
}

struct fti_vec *fti_vec_create(size_t obj_size, void *userctx, void *(*obj_init)(void *userctx, void *ptr), void (*obj_destroy)(void *userctx, void *ptr)) {
    struct fti_vec *vec = ft_user_alloc(sizeof(*vec));
    fti_vec_init(vec, obj_size, userctx, obj_init, obj_destroy);
    return vec;   
}

void fti_vec_destroy(struct fti_vec *vec) {
    fti_vec_deinit(vec);
    ft_user_free(vec, sizeof(*vec));
}

void fti_vec_init(struct fti_vec *vec, size_t obj_size, void *userctx, void *(*obj_init)(void *userctx, void *ptr), void (*obj_destroy)(void *userctx, void *ptr)) {
    vec->obj_size = obj_size;
    vec->userctx = userctx;
    vec->obj_init = obj_init;
    vec->obj_destroy = obj_destroy;

    vec->size = 0;
    vec->capacity = 0;
    vec->data = NULL;
}

void fti_vec_deinit(struct fti_vec *vec) {
    fti_vec_clear(vec);
    vec_reallocate(vec, 0);
}

void fti_vec_reserve(struct fti_vec *vec, size_t min_capacity) {
    if (min_capacity <= vec->capacity) {
        return;
    }
    vec_reallocate(vec, min_capacity);
}

void fti_vec_shrink_to_fit(struct fti_vec *vec) {
    vec_reallocate(vec, vec->size);
}

void fti_vec_erase_range(struct fti_vec *vec, size_t idx_first, size_t idx_last) {
    if (idx_first > idx_last) {
        return;
    }
    size_t count = (idx_last - idx_first) + 1;
    if (vec->obj_destroy != NULL) {
        for (size_t i = 0; i < count; i++) {
            vec->obj_destroy(vec->userctx, fti_vec_idx(vec, i));
        }
    }
    if ((idx_last + 1) < vec->size) {
        memmove(fti_vec_idx(vec, idx_first), fti_vec_idx(vec, idx_last + 1), (vec->size - (idx_last + 1)) * vec->obj_size);
    }
    vec->size -= count;
}

void fti_vec_clear(struct fti_vec *vec) {
    fti_vec_erase_range(vec, 0, vec->size - 1);
}

void* fti_vec_push_back(struct fti_vec *vec) {
    vec_ensure_size(vec, vec->size + 1);
    void *ptr = fti_vec_idx(vec, vec->size - 1);
    if (vec->obj_init != NULL) {
        vec->obj_init(vec->userctx, ptr);
    }
    return ptr;
}

void fti_vec_pop_back(struct fti_vec *vec) {
    fti_vec_erase(vec, vec->size - 1);
}
