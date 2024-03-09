//
// Created by Mahamadou DOUMBIA [OML DSI] on 22/02/2024.
//

#ifndef MOSCC_MSC_USSOCKET_H
#define MOSCC_MSC_USSOCKET_H

#include <libusockets.h>
#ifdef __cplusplus
extern "C"
{
#endif

void msc_socket_on_open(int ssl, struct us_socket_context_t *context,
                  struct us_socket_t *(*handler)(struct us_socket_t *, int, char *, int, void *), void *userData);

void msc_socket_on_writable(int ssl, struct us_socket_context_t *context,
                      struct us_socket_t *(*handler)(struct us_socket_t *, void *), void *userData);

void msc_socket_on_data(int ssl, struct us_socket_context_t *context,
                  struct us_socket_t *(*handler)(struct us_socket_t *, char *, int, void *), void *userData);

void msc_socket_on_close(int ssl, struct us_socket_context_t *context,
                   struct us_socket_t *(*handler)(struct us_socket_t *, int, void *, void *), void *userData);

void msc_socket_on_connect_error(int ssl, struct us_socket_context_t *context,
                          struct us_socket_t *(*handler)(struct us_socket_t *, int, void *), void *userData);
void msc_socket_on_end(int ssl, struct us_socket_context_t *context,
                 struct us_socket_t *(*handler)(struct us_socket_t *, void *), void *userData);

void msc_socket_on_timeout(int ssl, struct us_socket_context_t *context,
                    struct us_socket_t *(*handler)(struct us_socket_t *, void *), void *userData);

void msc_socket_on_long_timeout(int ssl, struct us_socket_context_t *context,
                        struct us_socket_t *(*handler)(struct us_socket_t *, void *), void *userData);

void msc_socket_on_server_name(int ssl, struct us_socket_context_t *context,
                        void (*handler)(struct us_socket_context_t *, const char *, void *),
                        void *userData);

void msc_socket_context_close(int ssl, struct us_socket_context_t *context);
struct us_socket_context_t *msc_socket_create(int ssl, struct us_loop_t *loop, int extSize, struct us_socket_context_options_t options);

struct us_socket_t *
msc_socket_connect(int ssl, struct us_socket_context_t *context, const char *host, int port, const char *sourceHost,
              int options, int socketSize);

struct us_listen_socket_t *
msc_socket_listen(int ssl, struct us_socket_context_t *context, const char *host, int port, int options,
             int socketSize);
struct us_listen_socket_t *
msc_socket_listen_unix(int ssl, struct us_socket_context_t *context, const char *path, int options,
             int socketSize);


#ifdef __cplusplus
extern "C"
}
#endif

#endif //MOSCC_MSC_USSOCKET_H
