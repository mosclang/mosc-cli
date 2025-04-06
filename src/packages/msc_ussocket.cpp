//
// Created by Mahamadou DOUMBIA [OML DSI] on 22/02/2024.
//

#ifndef MOSCC_MSC_USSOCKET_H
#define MOSCC_MSC_USSOCKET_H

#include <libusockets.h>

extern "C" {




void msc_socket_context_close(int ssl, struct us_socket_context_t *context) {
    us_socket_context_close(ssl, context);
}

us_socket_context_t *msc_socket_create(int ssl, us_loop_t *loop, int extSize, us_socket_context_options_t options) {
    return us_create_socket_context(ssl, loop, extSize, options);
}

struct us_socket_t *
msc_socket_connect(int ssl, struct us_socket_context_t *context, const char *host, int port, const char *sourceHost,
                   int options, int socketSize) {
    return us_socket_context_connect(ssl, context, host, port, sourceHost, options, socketSize);
}

struct us_listen_socket_t *
msc_socket_listen(int ssl, struct us_socket_context_t *context, const char *host, int port, int options,
                  int socketSize) {
    return us_socket_context_listen(ssl, context, host, port, options, socketSize);
}
struct us_listen_socket_t *
msc_socket_listen_unix(int ssl, struct us_socket_context_t *context, const char *path, int options,
                       int socketSize) {
    return us_socket_context_listen_unix(ssl, context, path, options, socketSize);
}
}
#endif //MOSCC_MSC_USSOCKET_H
