//
// Created by Mahamadou DOUMBIA [OML DSI] on 05/03/2024.
//
#include "msc.h"
#include "runtime.h"
#include "../../deps/mosc/src/memory/Value.h"

// ############################################################### Socket related functions #####################################################################

static MSCHandle *socketClass;
static MSCHandle *fnCall;
static MSCHandle *fnCall1;
static MSCHandle *fnCall2;

void shutdownUVSocket() {
    MVM *vm = getVM();
    if (socketClass) {
        MSCReleaseHandle(vm, socketClass);
    }
    if (fnCall) {
        MSCReleaseHandle(vm, fnCall);
    }
    if (fnCall1) {
        MSCReleaseHandle(vm, fnCall1);
    }
    if (fnCall2) {
        MSCReleaseHandle(vm, fnCall2);
    }
}

void initUVSocket(Djuru *djuru) {
    MVM *vm = getVM();
    MSCGetVariable(djuru, "net", "UVSocket", 0);
    socketClass = MSCGetSlotHandle(djuru, 0);
    fnCall = MSCMakeCallHandle(vm, "weele()");
    fnCall1 = MSCMakeCallHandle(vm, "weele(_)");
    fnCall2 = MSCMakeCallHandle(vm, "weele(_,_)");
    registerForShutdown(shutdownUVSocket);
}


typedef struct {
    int ssl;
    bool closed;
    MSCHandle *acceptEvent;
    MSCHandle *connectEvent;
    MSCHandle *dataEvent;
    MSCHandle *errorEvent;
} SocketData;
typedef struct {
    uv_tcp_t *socket;
    uv_buf_t buffer;
    MSCHandle *guard;
} SocketWriteRequest;

void uvSocketInit(Djuru *djuru) {
    MSCSetSlotHandle(djuru, 0, socketClass);
    uv_tcp_t *socket = MSCSetSlotNewExtern(djuru, 0, 0, sizeof(uv_tcp_t));
    SocketData *data = malloc(sizeof(SocketData));
    uv_tcp_init(getLoop(), socket);
    socket->data = data;
    data->ssl = 1;
    data->acceptEvent = NULL;
    data->dataEvent = NULL;
    data->errorEvent = NULL;
    data->connectEvent = NULL;
    data->closed = false;
    if (MSCGetSlotType(djuru, 1) != MSC_TYPE_MAP) {
        return;
    }
    int slotCount = MSCGetSlotCount(djuru);
    MSCEnsureSlots(djuru, 3);
    MSCSetSlotString(djuru, 2, "accept");
    MSCGetMapValue(djuru, 1, 2, 2);
    if (MSCGetSlotType(djuru, 2) != MSC_TYPE_NULL) {
        data->acceptEvent = MSCGetSlotHandle(djuru, 2);
    }
    MSCSetSlotString(djuru, 2, "data");
    MSCGetMapValue(djuru, 1, 2, 2);
    if (MSCGetSlotType(djuru, 2) != MSC_TYPE_NULL) {
        data->dataEvent = MSCGetSlotHandle(djuru, 2);
    }
    MSCSetSlotString(djuru, 2, "connect");
    MSCGetMapValue(djuru, 1, 2, 2);
    if (MSCGetSlotType(djuru, 2) != MSC_TYPE_NULL) {
        data->connectEvent = MSCGetSlotHandle(djuru, 2);
    }
    MSCSetSlotString(djuru, 2, "error");
    MSCGetMapValue(djuru, 1, 2, 2);
    if (MSCGetSlotType(djuru, 2) != MSC_TYPE_NULL) {
        data->errorEvent = MSCGetSlotHandle(djuru, 2);
    }
    MSCEnsureSlots(djuru, slotCount);

}

static void socketCloseCB(uv_handle_t *handle) {}

void uvSocketDestroy(void *handle) {
    uv_tcp_t *socket = (uv_tcp_t *) handle;
    SocketData *data = socket->data;
    if (data == NULL) {
        return;
    }
    if (!data->closed) {
        uv_close((uv_handle_t *) socket, socketCloseCB);
    }
    MVM *vm = getVM();
    if (data->acceptEvent != NULL) {
        MSCReleaseHandle(vm, data->acceptEvent);
    }
    if (data->connectEvent != NULL) {
        MSCReleaseHandle(vm, data->connectEvent);
    }
    if (data->dataEvent != NULL) {
        MSCReleaseHandle(vm, data->dataEvent);
    }
    if (data->errorEvent != NULL) {
        MSCReleaseHandle(vm, data->errorEvent);
    }
    free(data);
}

void uvRaiseSockerError(SocketData *data, int code, const char *source) {
    if (data->errorEvent == NULL) {
        return;
    }
    Djuru *djuru = getCurrentThread();
    MSCEnsureSlots(djuru, 3);
    MSCSetSlotHandle(djuru, 0, data->errorEvent);
    MSCSetSlotDouble(djuru, 1, code);
    MSCSetSlotString(djuru, 2, source);
    MSCCall(djuru, fnCall2);
}

void uvSocketBind(Djuru *djuru) {
    const char *ip = MSCGetSlotString(djuru, 1);
    int port = (int) MSCGetSlotDouble(djuru, 2);
    uv_tcp_t *socket = (uv_tcp_t *) MSCGetSlotExtern(djuru, 0);
    struct sockaddr_in addr;
    uv_ip4_addr(ip, port, &addr);

    int res = uv_tcp_bind(socket, (const struct sockaddr *) &addr, 0);
    MSCSetSlotDouble(djuru, 0, res);
}

void socketOnAcceptCallBack(uv_stream_t *s, int status) {
    uv_tcp_t *socket = (uv_tcp_t *) s;
    SocketData *data = (SocketData *) socket->data;
    if (status < 0) {
        uvRaiseSockerError(data, status, "ACCEPT");
        return;
    }
    if (data->acceptEvent == NULL) {
        return;
    }
    Djuru *djuru = getCurrentThread();
    MSCEnsureSlots(djuru, 1);
    MSCSetSlotHandle(djuru, 0, data->acceptEvent);
    MSCCall(djuru, fnCall);
}

void uvSocketListen(Djuru *djuru) {
    uv_tcp_t *socket = (uv_tcp_t *) MSCGetSlotExtern(djuru, 0);
    int backlog = (int) MSCGetSlotDouble(djuru, 1);
    int res = uv_listen((uv_stream_t *) socket, backlog, socketOnAcceptCallBack);
    MSCSetSlotDouble(djuru, 0, res);
}

void uvSocketAccept(Djuru *djuru) {
    uv_stream_t *server = (uv_stream_t *) MSCGetSlotExtern(djuru, 0);
    uv_stream_t *client = (uv_stream_t *) MSCGetSlotExtern(djuru, 1);
    int res = uv_accept(server, client);
    MSCSetSlotDouble(djuru, 0, res);
}

void socketAllocCb(uv_handle_t *handle,
                   size_t suggestedSize,
                   uv_buf_t *buf) {
    buf->base = (char *) malloc(suggestedSize);
    buf->len = suggestedSize;
}


void socketReadCb(uv_stream_t *stream,
                  ssize_t nread,
                  const uv_buf_t *buf) {
    uv_tcp_t *socket = (uv_tcp_t *) stream;
    SocketData *data = (SocketData *) socket->data;
    if (nread < 0) {
        if (nread != UV_EOF) {
            free(buf->base);
            uvRaiseSockerError(data, (int) nread, "READ");
            return;
        }
    }
    if (data->dataEvent == NULL) {
        free(buf->base);
        return;
    }
    Djuru *djuru = getCurrentThread();
    MSCEnsureSlots(djuru, 2);
    MSCSetSlotHandle(djuru, 0, data->dataEvent);
    if (nread == UV_EOF) {
        MSCSetSlotNull(djuru, 1);
    } else {
        MSCSetSlotBytes(djuru, 1, buf->base, (size_t) nread);
    }
    free(buf->base);
    MSCCall(djuru, fnCall1);
}


void uvSocketRead(Djuru *djuru) {
    uv_stream_t *socket = (uv_stream_t *) MSCGetSlotExtern(djuru, 0);
    int res = uv_read_start(socket, socketAllocCb, socketReadCb);
    MSCSetSlotDouble(djuru, 0, res);
}

void socketConnectCb(uv_connect_t *connect, int status) {
    uv_tcp_t *socket = (uv_tcp_t *) connect->handle;
    SocketData *data = (SocketData *) socket->data;
    if (status < 0) {
        uvRaiseSockerError(data, status, "CONNECT");
        return;
    }
    if (data->connectEvent == NULL) {
        return;
    }
    Djuru *djuru = getCurrentThread();
    MSCEnsureSlots(djuru, 1);
    MSCSetSlotHandle(djuru, 0, data->connectEvent);
    MSCCall(djuru, fnCall);
    free(connect);
}

void uvSocketConnect(Djuru *djuru) {
    uv_tcp_t *socket = (uv_tcp_t *) MSCGetSlotExtern(djuru, 0);
    uv_connect_t *connect = (uv_connect_t *) malloc(sizeof(uv_connect_t));
    const char *ip = MSCGetSlotString(djuru, 1);
    int port = (int) MSCGetSlotDouble(djuru, 2);

    struct sockaddr_in addr;
    uv_ip4_addr(ip, port, &addr);

    int res = uv_tcp_connect(connect, socket, (const struct sockaddr *) &addr, socketConnectCb);
    MSCSetSlotDouble(djuru, 0, res);
}

void socketWriteCb(uv_write_t *r, int status) {

    SocketWriteRequest *req = (SocketWriteRequest *) r->data;

    SocketData *data = (SocketData *) req->socket->data;
    if (req->guard != NULL) {
        MSCReleaseHandle(getVM(), req->guard);
    }
    free(req);
    free(r);
    if (status < 0) {
        uvRaiseSockerError(data, status, "WRITE");
        return;
    }

}

void uvSocketWrite(Djuru *djuru) {
    uv_stream_t *socket = (uv_stream_t *) MSCGetSlotExtern(djuru, 0);
    uv_write_t *req = (uv_write_t *) malloc(sizeof(uv_write_t));
    SocketWriteRequest *socketWriteReq = (SocketWriteRequest *) malloc(sizeof(uv_write_t));
    req->data = socketWriteReq;

    int length;
    const char *data = MSCGetSlotBytes(djuru, 1, &length);
    socketWriteReq->guard = MSCGetSlotHandle(djuru, 1); // keep data so that it's not garbage collected
    socketWriteReq->buffer = uv_buf_init((char *) data, (unsigned int) length);
    socketWriteReq->socket = (uv_tcp_t *) socket;
    uv_write(req, socket, &socketWriteReq->buffer, 1, socketWriteCb);
}

void uvSocketClose(Djuru *djuru) {
    uv_tcp_t *socket = (uv_tcp_t *) MSCGetSlotExtern(djuru, 0);
    uv_close((uv_handle_t *) socket, NULL);
    ((SocketData *) socket->data)->closed = true;

}