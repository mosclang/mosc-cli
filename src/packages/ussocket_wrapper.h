//
// Created by Mahamadou DOUMBIA [OML DSI] on 22/02/2024.
//

#ifndef MOSCC_USSOCKET_WRAPPER_H
#define MOSCC_USSOCKET_WRAPPER_H

#include "msc_ussocket.h"

extern "C" {
void msc_socket_on_open(int ssl, struct us_socket_context_t *context,
                        struct us_socket_t *(*handler)(struct us_socket_t *, int, char *, int, void *),
                        void *userData) {
    return msc::socketOnOpen(ssl, context, handler, userData);
}
void msc_socket_on_data(int ssl, struct us_socket_context_t *context,
                        struct us_socket_t *(*handler)(struct us_socket_t *, char *, int, void *), void *userData) {
    return msc::socketOnData(ssl, context, handler, userData);
}
void msc_socket_on_writable(int ssl, struct us_socket_context_t *context,
                            struct us_socket_t *(*handler)(struct us_socket_t *, void *), void *userData) {
    return msc::socketOnWritable(ssl, context, handler, userData);
}
void msc_socket_on_connect_error(int ssl, struct us_socket_context_t *context,
                                 struct us_socket_t *(*handler)(struct us_socket_t *, int, void *), void *userData) {
    return msc::socketOnConnectError(ssl, context, handler, userData);
}
void msc_socket_on_close(int ssl, struct us_socket_context_t *context,
                         struct us_socket_t *(*handler)(struct us_socket_t *, int, void *, void *), void *userData) {
    return msc::socketOnClose(ssl, context, handler, userData);
}
void msc_socket_on_end(int ssl, struct us_socket_context_t *context,
                       struct us_socket_t *(*handler)(struct us_socket_t *, void *), void *userData) {
    return msc::socketOnEnd(ssl, context, handler, userData);
}
void msc_socket_on_timeout(int ssl, struct us_socket_context_t *context,
                           struct us_socket_t *(*handler)(struct us_socket_t *, void *), void *userData) {
    return msc::socketOnTimout(ssl, context, handler, userData);
}
void msc_socket_on_long_timeout(int ssl, struct us_socket_context_t *context,
                                struct us_socket_t *(*handler)(struct us_socket_t *, void *), void *userData) {
    return msc::socketOnLongTimout(ssl, context, handler, userData);
}
void msc_socket_on_servername(int ssl, struct us_socket_context_t *context,
                              void (*handler)(struct us_socket_context_t *, const char *, void *), void *userData) {
    return msc::socketOnServerName(ssl, context, handler, userData);
}
struct us_socket_context_t *
msc_create_socket_context(int ssl, us_loop_t *loop, int extSize, us_socket_context_options_t options) {
    return msc::socketCreate(ssl, loop, extSize, options);
}
void msc_socket_context_close(int ssl, struct us_socket_context_t *context) {
    return msc::socketClose(ssl, context);
}
struct us_socket_t *msc_socket_context_connect(int ssl, struct us_socket_context_t *context, const char *host, int port,
                                               const char *sourceHost,
                                               int options, int socketSize) {
    return msc::socketConnect(ssl, context, host, port, sourceHost, options, socketSize);
}
struct us_listen_socket_t *
msc_socket_context_listent(int ssl, struct us_socket_context_t *context, const char *host, int port,
                           int options, int socketSize) {
    return msc::socketListen(ssl, context, host, port, options, socketSize);
}
};
#endif //MOSCC_USSOCKET_WRAPPER_H
