#include <tweetnacl.h>
#include "ft.h"
#include "protoparse.h"

// https://github.com/markqvist/Reticulum/blob/1565126ffd08b9d7bc750ce5df82d5aa3e38183e/RNS/Packet.py#L253-L257
#define RETICULUM_FLAG_PACKET_TYPE(x) ((x) & 0x3)
#define RETICULUM_FLAG_DESTINATION_TYPE(x) (((x) >> 2) & 0x3)
#define RETICULUM_FLAG_PROPAGATION_TYPE(x) (((x) >> 4) & 0x1)
#define RETICULUM_FLAG_CONTEXT(x) (((x) >> 5) & 0x1)
#define RETICULUM_FLAG_HEADER_TYPE(x) (((x) >> 6) & 0x1)
#define RETICULUM_FLAG_IFAC(x) (((x) >> 7) & 0x1)

int fti_parse_header(void *data, size_t len, struct reticulum_header *header) {
    // https://github.com/markqvist/Reticulum/blob/1565126ffd08b9d7bc750ce5df82d5aa3e38183e/RNS/Packet.py#L246-L279
    uint8_t *data_u8 = (uint8_t *)data;

    size_t idx = 0;

    // flags
    if (len < (idx + 1)) {
        return -1;
    }
    uint8_t flags = data_u8[idx];
    header->packet_type = RETICULUM_FLAG_PACKET_TYPE(flags);
    header->destination_type = RETICULUM_FLAG_DESTINATION_TYPE(flags);
    header->propagation_type = RETICULUM_FLAG_PROPAGATION_TYPE(flags);
    header->context_flag = RETICULUM_FLAG_CONTEXT(flags) != 0;
    idx += 1;

    // hops
    if (len < (idx + 1)) {
        return -1;
    }
    header->hops = data_u8[idx];
    idx += 1;

    // we should _not_ have IFAC at this stage
    if (RETICULUM_FLAG_IFAC(flags)) {
        return -2;
    }

    // transport ID (comes first if we have two destination hashes)
    if (RETICULUM_FLAG_HEADER_TYPE(flags) == 1) {
        if (len < (idx + sizeof(reticulum_destination_t))) {
            return -1;
        }
        header->transport_id = (reticulum_destination_t *)&data_u8[idx];
        idx += sizeof(reticulum_destination_t);
    } else {
        header->transport_id = NULL;
    }

    // destination hash
    if (len < (idx + sizeof(reticulum_destination_t))) {
        return -1;
    }
    header->dest = (reticulum_destination_t *)&data_u8[idx];
    idx += sizeof(reticulum_destination_t);

    // context byte
    if (len < (idx + 1)) {
        return -1;
    }
    header->context = data_u8[idx];
    idx += 1;

    // data
    header->data = &data_u8[idx];
    header->data_len = len - idx;
    return 0;
}

int fti_parse_announce(struct reticulum_header *header, struct reticulum_announce *announce) {
    // https://github.com/markqvist/Reticulum/blob/1565126ffd08b9d7bc750ce5df82d5aa3e38183e/RNS/Identity.py#L513-L612
    uint8_t *data_u8 = (uint8_t *)header->data;

    size_t idx = 0;

    announce->dest = header->dest;

    // key
    if (header->data_len < (idx + (RETICULUM_ID_KEY_BITS / 8))) {
        return -1;
    }
    announce->key = (uint8_t (*)[RETICULUM_ID_KEY_BITS / 8])&data_u8[idx];
    idx += RETICULUM_ID_KEY_BITS / 8;

    // name hash
    if (header->data_len < (idx + (RETICULUM_ID_NAME_HASH_BITS / 8))) {
        return -1;
    }
    announce->name_hash = (uint8_t (*)[RETICULUM_ID_NAME_HASH_BITS / 8])&data_u8[idx];
    idx += RETICULUM_ID_NAME_HASH_BITS / 8;

    // random hash
    if (header->data_len < (idx + RETICULUM_ID_RANDOM_HASH_BYTES)) {
        return -1;
    }
    announce->random_hash = (uint8_t (*)[RETICULUM_ID_RANDOM_HASH_BYTES])&data_u8[idx];
    idx += RETICULUM_ID_RANDOM_HASH_BYTES;

    // ratchet (if context flag set)
    if (header->context_flag) {
        if (header->data_len < (idx + (RETICULUM_ID_RATCHET_BITS / 8))) {
            return -1;
        }
        announce->ratchet = (uint8_t (*)[RETICULUM_ID_RATCHET_BITS / 8])&data_u8[idx];
        idx += RETICULUM_ID_RATCHET_BITS / 8;
    } else {
        announce->ratchet = NULL;
    }

    // signature
    if (header->data_len < (idx + (RETICULUM_ID_SIGNATURE_BITS / 8))) {
        return -1;
    }
    announce->signature = (uint8_t (*)[RETICULUM_ID_SIGNATURE_BITS / 8])&data_u8[idx];
    idx += RETICULUM_ID_SIGNATURE_BITS / 8;

    // app data
    announce->app_data_len = header->data_len - idx;
    if (announce->app_data_len > 0) {
        announce->app_data = &data_u8[idx];
    } else {
        announce->app_data = NULL;
    }

    return 0;
}

int fti_validate_announce_sig(struct reticulum_announce *announce) {
    size_t smlen = (
        sizeof(*announce->signature)
        + sizeof(*announce->dest)
        + sizeof(*announce->key)
        + sizeof(*announce->name_hash)
        + sizeof(*announce->random_hash)
        + (announce->ratchet != NULL ? sizeof(*announce->ratchet) : 0)
        + (announce->app_data != NULL ? announce->app_data_len : 0)
    );
    uint8_t *sm = ft_user_alloc(smlen);
    size_t smidx = 0;
    memcpy(&sm[smidx], announce->signature, sizeof(*announce->signature));
    smidx += sizeof(*announce->signature);
    memcpy(&sm[smidx], announce->dest, sizeof(*announce->dest));
    smidx += sizeof(*announce->dest);
    memcpy(&sm[smidx], announce->key, sizeof(*announce->key));
    smidx += sizeof(*announce->key);
    memcpy(&sm[smidx], announce->name_hash, sizeof(*announce->name_hash));
    smidx += sizeof(*announce->name_hash);
    memcpy(&sm[smidx], announce->random_hash, sizeof(*announce->random_hash));
    smidx += sizeof(*announce->random_hash);
    if (announce->ratchet != NULL) {
        memcpy(&sm[smidx], announce->ratchet, sizeof(*announce->ratchet));
        smidx += sizeof(*announce->ratchet);
    }
    if (announce->app_data != NULL) {
        memcpy(&sm[smidx], announce->app_data, announce->app_data_len);
        smidx += announce->app_data_len;
    }

    uint8_t *m = ft_user_alloc(smlen);

    unsigned long long mlen;
    int cryptostatus = crypto_sign_open(m, &mlen, sm, smlen, (unsigned char *)&(*announce->key)[(RETICULUM_ID_KEY_BITS / 8) / 2]);

    ft_user_free(sm, smlen);
    ft_user_free(m, smlen);

    return cryptostatus == 0 ? 0 : -1;
}

static int rand() {
    static int rand_seed = 1;
    rand_seed = (42069 * rand_seed + 1) % (65535 + 1);
    if (rand_seed < 0) { // make sure the result is always positive
        rand_seed = -rand_seed;
    }
    return rand_seed;
}

// for tweetnacl
void randombytes(unsigned char *data, unsigned long long len) {
    // FIXME: BUG: world's most secure crypto
    for (unsigned long long i = 0; i < len; i++) {
        data[i] = rand();
    }
}
