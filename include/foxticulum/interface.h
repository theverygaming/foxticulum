#pragma once
#include <stddef.h>
#include <foxticulum/foxticulum.h>

typedef void* ft_interface_handle;

ft_interface_handle ft_interface_register(ft_handle, void (*tx)(void *userdata, void *data, size_t len), void *userdata);
void ft_interface_rx(ft_handle, ft_interface_handle interface, void *data, size_t len);
void ft_interface_deregister(ft_handle, ft_interface_handle interface);
