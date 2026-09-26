#include <stdint.h>
#include "ft.h"
#include "protoparse.h"

ft_interface_handle ft_interface_register(ft_handle ft, void (*tx)(void *userdata, void *data, size_t len), void *userdata) {}

void ft_interface_rx(ft_handle ft, ft_interface_handle interface, void *data, size_t len) {
    FT_LOG(FT_LOG_DEBUG, "got packet of %zu bytes\n", len);

    struct reticulum_header_ptrs header;
    int err;
    if ((err = fti_parse_header(data, len, &header)) < 0) {
        FT_LOG(FT_LOG_ERR, "error parsing reticulum header: %d\n", err);
        return;
    }

    FT_LOG(FT_LOG_DEBUG, "  flags:\n");
    FT_LOG(FT_LOG_DEBUG, "    packet type: %u\n", header.packet_type);
    FT_LOG(FT_LOG_DEBUG, "    destination type: %u\n", header.destination_type);
    FT_LOG(FT_LOG_DEBUG, "    propagation type: %u\n", header.propagation_type);
    FT_LOG(FT_LOG_DEBUG, "    context flag: %c\n", header.context_flag ? 'Y' : 'N');
    FT_LOG(FT_LOG_DEBUG, "  hops: %u\n", header.hops);
    if (header.transport_id != NULL) {
        FT_LOG_BYTES(FT_LOG_DEBUG,"  transport: <", ">\n", "%02x", *header.transport_id, sizeof(*header.transport_id));
    }
    FT_LOG_BYTES(FT_LOG_DEBUG, "  destination: <", ">\n", "%02x", *header.dest, sizeof(*header.dest));
    FT_LOG(FT_LOG_DEBUG, "  context: 0x%02x\n", header.context);

    if (header.packet_type == PACKET_ANNOUNCE) {
        struct reticulum_announce_ptrs announce;
        if ((err = fti_parse_announce(&header, &announce)) < 0) {
            FT_LOG(FT_LOG_ERR, "error parsing reticulum announce: %d\n", err);
            return;
        }
        FT_LOG(FT_LOG_DEBUG, "  announce:\n");
        if ((err = fti_validate_announce_sig(&announce)) < 0) {
            FT_LOG(FT_LOG_ERR, "    signature invalid (err %d)\n", err);
        } else {
            FT_LOG(FT_LOG_ERR, "    signature valid\n");
        }
        if (announce.app_data != NULL) {
            char *astr = ft_user_alloc(announce.app_data_len + 1);
            memcpy(astr, announce.app_data, announce.app_data_len);
            astr[announce.app_data_len] = '\0';
            FT_LOG(FT_LOG_DEBUG, "    as string: '%s'\n", astr);
            ft_user_free(astr, announce.app_data_len + 1);
        }
    }
}

void ft_interface_deregister(ft_handle ft, ft_interface_handle interface) {}
