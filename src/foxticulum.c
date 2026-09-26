#include "ft.h"

ft_handle ft_init(struct ft_config config) {
    FT_LOG(FT_LOG_DEBUG, "init\n");
    struct foxticulum_handle *handle = (ft_user_alloc(sizeof(struct foxticulum_handle)));
    handle->config = config;
    return handle;
}

void ft_deinit(ft_handle ft) {
    ft_user_free(ft, sizeof(struct foxticulum_handle));
}
