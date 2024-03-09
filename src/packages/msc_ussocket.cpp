//
// Created by Mahamadou DOUMBIA [OML DSI] on 22/02/2024.
//

#ifndef MOSCC_MSC_USSOCKET_H
#define MOSCC_MSC_USSOCKET_H

#include <libusockets.h>

extern "C" {

void msc_socket_on_open(int ssl, struct us_socket_context_t *context,
                        struct us_socket_t *(*handler)(struct us_socket_t *, int, char *, int, void *),
                        void *userData) {
    /*us_socket_context_on_open(ssl, context, [handler, userData](struct us_socket_t *s, int is_client, char *ip,
                                                                int ip_length) -> struct us_socket_t * {
        return handler(s, is_client, ip, ip_length, userData);
    });*/
}


void msc_socket_on_writable(int ssl, struct us_socket_context_t *context,
                            struct us_socket_t *(*handler)(struct us_socket_t *, void *), void *userData) {
    /*us_socket_context_on_writable(ssl, context, [handler, userData](struct us_socket_t *s) -> struct us_socket_t * {
        return handler(s, userData);
    });*/
}

void msc_socket_on_data(int ssl, struct us_socket_context_t *context,
                        struct us_socket_t *(*handler)(struct us_socket_t *, char *, int, void *), void *userData) {
    /*us_socket_context_on_data(ssl, context,
                              [handler, userData](struct us_socket_t *s, char *data, int size) -> struct us_socket_t * {
                                  return handler(s, data, size, userData);
                              });*/
}

void msc_socket_on_close(int ssl, struct us_socket_context_t *context,
                         struct us_socket_t *(*handler)(struct us_socket_t *, int, void *, void *), void *userData) {
    /*us_socket_context_on_close(ssl, context,
                               [handler, userData](struct us_socket_t *s, int code,
                                                   void *reason) -> struct us_socket_t * {
                                   return handler(s, code, reason, userData);
                               });*/
}

void msc_socket_on_connect_error(int ssl, struct us_socket_context_t *context,
                                 struct us_socket_t *(*handler)(struct us_socket_t *, int, void *), void *userData) {
    /*us_socket_context_on_connect_error(ssl, context,
                                       [handler, userData](struct us_socket_t *s, int code) -> struct us_socket_t * {
                                           return handler(s, code, userData);
                                       });*/
}

void msc_socket_on_end(int ssl, struct us_socket_context_t *context,
                       struct us_socket_t *(*handler)(struct us_socket_t *, void *), void *userData) {
    /*us_socket_context_on_end(ssl, context, [handler, userData](struct us_socket_t *s) -> struct us_socket_t * {
        return handler(s, userData);
    });*/
}

void msc_socket_on_timeout(int ssl, struct us_socket_context_t *context,
                           struct us_socket_t *(*handler)(struct us_socket_t *, void *), void *userData) {
    /*us_socket_context_on_timeout(ssl, context, [handler, userData](struct us_socket_t *s) -> struct us_socket_t * {
        return handler(s, userData);
    });*/
}

void msc_socket_on_long_timeout(int ssl, struct us_socket_context_t *context,
                                struct us_socket_t *(*handler)(struct us_socket_t *, void *), void *userData) {
    /*us_socket_context_on_long_timeout(ssl, context, [handler, userData](struct us_socket_t *s) -> struct us_socket_t * {
        return handler(s, userData);
    });*/
}

void msc_socket_on_server_name(int ssl, struct us_socket_context_t *context,
                               void (*handler)(struct us_socket_context_t *, const char *, void *),
                               void *userData) {
    /*us_socket_context_on_server_name(ssl, context,
                                     [handler, userData](struct us_socket_context_t *c, const char *name) -> void {
                                         handler(c, name, userData);
                                     });*/
}

void msc_socket_context_close(int ssl, struct us_socket_context_t *context) {
    // us_socket_context_close(ssl, context);
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
