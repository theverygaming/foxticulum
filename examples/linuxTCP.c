#include <stdlib.h>
#include <stdio.h>
#include <sys/socket.h>
#include <netinet/ip.h>
#include <string.h>
#include <netdb.h>
#include <unistd.h>
#include <stdbool.h>
#include <stdarg.h>
#include <foxticulum/foxticulum.h>
#include <foxticulum/interface.h>
#include <foxticulum/user.h>

// https://stackoverflow.com/a/3599170
#define PARAM_UNUSED(x) (void)(x)

#define ERREXIT(msg) do { \
    fprintf(stderr, msg "\n"); \
    exit(1); \
} while(0)

#define CHKERR_ERRNO(x, msg) do { \
    if ((x) == -1) { \
        perror(msg); \
        exit(1); \
    } \
} while(0)

#define CHKERR_NEGATIVE(x, msg) do { \
    if ((x) < 0) { \
        ERREXIT(msg); \
    } \
} while(0)

void *ft_user_alloc(size_t n) {
    return malloc(n);
}

void *ft_user_realloc(void *ptr, size_t size_old, size_t size_new) {
    return realloc(ptr, size_new);
}

void ft_user_free(void *ptr, size_t n) {
    PARAM_UNUSED(n);
    free(ptr);
}

void ft_user_log(const char *fmt, ...) {
    va_list args;
    va_start(args, fmt);
    vfprintf(stderr, fmt, args);
    va_end(args);
}

// https://github.com/markqvist/Reticulum/blob/1565126ffd08b9d7bc750ce5df82d5aa3e38183e/RNS/Interfaces/TCPInterface.py#L44-L47
#define HDLC_FLAG 0x7E
#define HDLC_ESC 0x7D
#define HDLC_ESC_MASK 0x20

static ft_handle ft;
static ft_interface_handle ftif;

void dump_hex(void *data, size_t len) {
    for (size_t i = 0; i < len; i++) {
        fprintf(stderr, "0x%x%s", ((uint8_t*)data)[i], i != (len-1) ? " " : "\n");
    }
}

void rx_packet(void *data, size_t len) {
    fprintf(stderr, "RX: got packet of %zu bytes: ", len);
    dump_hex(data, len);
    ft_interface_rx(ft, ftif, data, len);
}

void send_packet(void *userdata, void *data, size_t len) {
    PARAM_UNUSED(userdata);
    fprintf(stderr, "TX: got packet of %zu bytes: ", len);
    dump_hex(data, len);
}

int main(int argc, const char **argv) {
    if (argc != 3) {
        fprintf(stderr, "usage: %s <host> <port>\n", argv[0]);
        exit(1);
    }
    const char *host = argv[1];
    long port = strtol(argv[2], NULL, 10);
    int tcp_socket = socket(AF_INET, SOCK_STREAM, 0);
    CHKERR_ERRNO(tcp_socket, "error creating socket");
    struct addrinfo *addrinfo;
    struct addrinfo hints;
    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;
    CHKERR_NEGATIVE(getaddrinfo(host, NULL, NULL, &addrinfo), "address lookup failed");
    struct sockaddr_in addr;
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_addr = ((struct sockaddr_in *)addrinfo->ai_addr)->sin_addr;
	addr.sin_port = htons(port);
    freeaddrinfo(addrinfo);

    CHKERR_ERRNO(connect(tcp_socket, (struct sockaddr *)&addr, sizeof(addr)), "connect failed");

    ft = ft_init((struct ft_config){
        .nothing = 1,
    });
    ftif = ft_interface_register(ft, &send_packet, NULL);

    // segmenting
    static uint8_t buf[4096];
    size_t buf_len = sizeof(buf);
    size_t buf_prev_offs = 0;
    // framing
    bool in_frame = false;
    size_t frame_start = 0;
    while (true) {
        uint8_t *buf_current = &buf[buf_prev_offs];
        size_t buf_current_len = buf_len - buf_prev_offs;
        int rleni = read(tcp_socket, buf_current, buf_current_len);
        CHKERR_NEGATIVE(rleni, "socket read error");
        size_t rlen = rleni;
	    if (rlen == 0) {
            ERREXIT("TCP Socket EOF");
            continue;
        }
        fprintf(stderr, "read %zu bytes from socket\n", rlen);
        // https://github.com/markqvist/Reticulum/blob/1565126ffd08b9d7bc750ce5df82d5aa3e38183e/RNS/Interfaces/TCPInterface.py#L389-L412
        // unescape and frame data
        size_t buf_current_len_data = buf_prev_offs + rlen;
        for (size_t i = buf_prev_offs; i < buf_current_len_data; i++) {
            if (buf[i] == HDLC_ESC) {
                if (i + 1 >= buf_current_len_data) {
                    // TODO: we'd have to somehow handle escapes spanning multiple read cycles...
                    // OOOOOOR we just handle escapes *after* the framing.. Maybe that would fix it (it would certainly make this loop simpler...)!
                    ERREXIT("unimplemented");
                }
                // recover original value
                buf[i + 1] ^= HDLC_ESC_MASK;
                // fix up hole
                memmove(&buf[i], &buf[i + 1], (buf_current_len_data - i) - 1);
                buf_current_len_data--;
            } else if (buf[i] == HDLC_FLAG) {
                if (in_frame) {
                    // skip the frame start flag
                    if (frame_start + 1 >= buf_current_len_data) {
                        // this shouldn't happen
                        ERREXIT("WTF???");
                    }
                    rx_packet(&buf[frame_start + 1], (i - frame_start) - 1);
                    in_frame = false;
                } else {
                    frame_start = i;
                    in_frame = true;
                }
            }
        }
        if (in_frame) {
            // TODO: with the test server I had I couldn't really seem to test HDLC frames spanning more than one read cycle..
            //fprintf(stderr, "in frame at loop end!\n");
            // copy the remaining bit of the frame to the start of the buffer
            size_t remaining_len = buf_current_len_data - frame_start;
            memmove(buf, &buf[frame_start], remaining_len);
            buf_prev_offs = remaining_len;
            frame_start = 0;
        } else {
            buf_prev_offs = 0;
        }
    }

    close(tcp_socket);
    ft_deinit(ft);

    return 0;
}
