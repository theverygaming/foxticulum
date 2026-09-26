#pragma once

struct ft_config {
    
};

typedef void * ft_handle;

ft_handle ft_init(struct ft_config config);
void ft_deinit(ft_handle);
