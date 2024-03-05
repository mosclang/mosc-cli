//
// Created by Mahamadou DOUMBIA [OML DSI] on 22/02/2024.
//

#ifndef MOSCC_MSC_USSOCKET_H
#define MOSCC_MSC_USSOCKET_H

#include <libusockets.h>

namespace msc {

    void socketOnOpen(int ssl, struct us_socket_context_t *context,
                      struct us_socket_t *(*handler)(struct us_socket_t *, int, char *, int, void *), void *userData) {
        us_socket_context_on_open(ssl, context, [handler, userData](auto s, auto is_client, auto ip,
                                                                    auto ip_length) -> struct us_socket_t * {
            return handler(s, is_client, ip, ip_length, userData);
        });
    }


    void socketOnWritable(int ssl, struct us_socket_context_t *context,
                          struct us_socket_t *(*handler)(struct us_socket_t *, void *), void *userData) {
        us_socket_context_on_writable(ssl, context, [handler, userData](auto s) -> struct us_socket_t * {
            return handler(s, userData);
        });
    }

    void socketOnData(int ssl, struct us_socket_context_t *context,
                      struct us_socket_t *(*handler)(struct us_socket_t *, char *, int, void *), void *userData) {
        us_socket_context_on_data(ssl, context,
                                  [handler, userData](auto s, auto data, auto size) -> struct us_socket_t * {
                                      return handler(s, data, size, userData);
                                  });
    }

    void socketOnClose(int ssl, struct us_socket_context_t *context,
                       struct us_socket_t *(*handler)(struct us_socket_t *, int, void *, void *), void *userData) {
        us_socket_context_on_close(ssl, context,
                                   [handler, userData](auto s, auto code, auto reason) -> struct us_socket_t * {
                                       return handler(s, code, reason, userData);
                                   });
    }

    void socketOnConnectError(int ssl, struct us_socket_context_t *context,
                              struct us_socket_t *(*handler)(struct us_socket_t *, int, void *), void *userData) {
        us_socket_context_on_connect_error(ssl, context,
                                           [handler, userData](auto s, auto code) -> struct us_socket_t * {
                                               return handler(s, code, userData);
                                           });
    }

    void socketOnEnd(int ssl, struct us_socket_context_t *context,
                     struct us_socket_t *(*handler)(struct us_socket_t *, void *), void *userData) {
        us_socket_context_on_end(ssl, context, [handler, userData](auto s) -> struct us_socket_t * {
            return handler(s, userData);
        });
    }

    void socketOnTimout(int ssl, struct us_socket_context_t *context,
                        struct us_socket_t *(*handler)(struct us_socket_t *, void *), void *userData) {
        us_socket_context_on_timeout(ssl, context, [handler, userData](auto s) -> struct us_socket_t * {
            return handler(s, userData);
        });
    }

    void socketOnLongTimout(int ssl, struct us_socket_context_t *context,
                            struct us_socket_t *(*handler)(struct us_socket_t *, void *), void *userData) {
        us_socket_context_on_long_timeout(ssl, context, [handler, userData](auto s) -> struct us_socket_t * {
            return handler(s, userData);
        });
    }

    void socketOnServerName(int ssl, struct us_socket_context_t *context,
                            void (*handler)(struct us_socket_context_t *, const char *, void *),
                            void *userData) {
        us_socket_context_on_server_name(ssl, context, [handler, userData](auto s, auto c) -> void {
            handler(s, c, userData);
        });
    }

    void socketClose(int ssl, struct us_socket_context_t *context) {
        us_socket_context_close(ssl, context);
    }

    us_socket_context_t *socketCreate(int ssl, us_loop_t *loop, int extSize, us_socket_context_options_t options) {
        return us_create_socket_context(ssl, loop, extSize, options);
    }

    struct us_socket_t *
    socketConnect(int ssl, struct us_socket_context_t *context, const char *host, int port, const char *sourceHost,
                  int options, int socketSize) {
        return us_socket_context_connect(ssl, context, host, port, sourceHost, options, socketSize);
    }

    struct us_listen_socket_t *
    socketListen(int ssl, struct us_socket_context_t *context, const char *host, int port, int options,
                 int socketSize) {
        return us_socket_context_listen(ssl, context, host, port, options, socketSize);
    }
}
#endif //MOSCC_MSC_USSOCKET_H
