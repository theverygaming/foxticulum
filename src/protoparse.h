#pragma once
#include <stdint.h>

typedef uint8_t reticulum_destination_t[16];

struct reticulum_header {
    enum {PACKET_DATA = 0, PACKET_ANNOUNCE = 1, PACKET_LINK_REQ = 2, PACKET_PROOF = 3} packet_type;
    enum {DESTINATION_SINGLE = 0, DESTINATION_GROUP = 1, DESTINATION_PLAIN = 2, DESTINATION_LINK = 3} destination_type;
    enum {PROPAGATION_BROADCAST = 0, PROPAGATION_TRANSPORT = 1} propagation_type;
    bool context_flag;
    uint8_t hops;
    reticulum_destination_t *transport_id; // may be NULL
    reticulum_destination_t *dest;
    enum {
        // https://github.com/markqvist/Reticulum/blob/192898864c008b6287dd56781d89fccef0bb5f7a/RNS/Packet.py#L72-L92
        CONTEXT_TYPE_NONE = 0, // generic data
        CONTEXT_TYPE_RESOURCE = 0x01,
        CONTEXT_TYPE_RESOURCE_ADV = 0x02,
        CONTEXT_TYPE_RESOURCE_REQ = 0x03,
        CONTEXT_TYPE_RESOURCE_HMU = 0x04,
        CONTEXT_TYPE_RESOURCE_PRF = 0x05,
        CONTEXT_TYPE_RESOURCE_ICL = 0x06,
        CONTEXT_TYPE_RESOURCE_RCL = 0x07,
        CONTEXT_TYPE_CACHE_REQUEST = 0x08,
        CONTEXT_TYPE_REQUEST = 0x09,
        CONTEXT_TYPE_RESPONSE = 0x0A,
        CONTEXT_TYPE_PATH_RESPONSE = 0x0B,
        CONTEXT_TYPE_COMMAND = 0x0C,
        CONTEXT_TYPE_COMMAND_STATUS = 0x0D,
        CONTEXT_TYPE_CHANNEL = 0x0E,
        CONTEXT_TYPE_KEEPALIVE = 0xFA,
        CONTEXT_TYPE_LINKIDENTIFY = 0xFB,
        CONTEXT_TYPE_LINKCLOSE = 0xFC,
        CONTEXT_TYPE_LINKPROOF = 0xFD,
        CONTEXT_TYPE_LRRTT = 0xFE,
        CONTEXT_TYPE_LRPROOF = 0xFF,
    } context;
    void *data;
    size_t data_len;
};

#define RETICULUM_ID_KEY_BITS (256 * 2) // https://github.com/markqvist/Reticulum/blob/1565126ffd08b9d7bc750ce5df82d5aa3e38183e/RNS/Identity.py#L59-L62
#define RETICULUM_ID_RATCHET_BITS 256 // https://github.com/markqvist/Reticulum/blob/1565126ffd08b9d7bc750ce5df82d5aa3e38183e/RNS/Identity.py#L64-L67
#define RETICULUM_ID_NAME_HASH_BITS 80 // https://github.com/markqvist/Reticulum/blob/1565126ffd08b9d7bc750ce5df82d5aa3e38183e/RNS/Identity.py#L83
#define RETICULUM_ID_SIGNATURE_BITS RETICULUM_ID_KEY_BITS // https://github.com/markqvist/Reticulum/blob/1565126ffd08b9d7bc750ce5df82d5aa3e38183e/RNS/Identity.py#L81
#define RETICULUM_ID_RANDOM_HASH_BYTES 10 // https://github.com/markqvist/Reticulum/blob/1565126ffd08b9d7bc750ce5df82d5aa3e38183e/RNS/Identity.py#L527

struct reticulum_announce {
    reticulum_destination_t *dest;
    uint8_t (*key)[RETICULUM_ID_KEY_BITS / 8];
    uint8_t (*name_hash)[RETICULUM_ID_NAME_HASH_BITS / 8];
    uint8_t (*random_hash)[RETICULUM_ID_RANDOM_HASH_BYTES];
    uint8_t (*ratchet)[RETICULUM_ID_RATCHET_BITS / 8]; // may be NULL
    uint8_t (*signature)[RETICULUM_ID_SIGNATURE_BITS / 8];
    void *app_data; // may be NULL
    size_t app_data_len;
};

int fti_parse_header(void *data, size_t len, struct reticulum_header *header);
int fti_parse_announce(struct reticulum_header *header, struct reticulum_announce *announce);
int fti_validate_announce_sig(struct reticulum_announce *announce);
