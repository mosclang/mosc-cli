//
// Created by Mahamadou DOUMBIA [OML DSI] on 28/12/2023.
//

#include <MVM.h>
#include <libuwebsockets.h>
#include "net.h"
#include "runtime.h"
#include "scheduler.h"


struct ByteArrayList {
    const char *data;
    size_t length;
    struct ByteArrayList *next;
};
typedef struct Chain Chain;
struct Chain {
    void *data;
    Chain* next;
};
static Chain* initChain(void* data) {
    Chain* chain = malloc(sizeof(Chain));
    chain->data = data;
    chain->next = NULL;
    return chain;
}
static Chain* insertInChain(Chain* chain, void* data) {
    if(!chain) {
        return initChain(data);
    }
    Chain* it = chain;
    while (it->next) {
        it = it->next;
    }
    it->next = malloc(sizeof(chain));
    it->next->data = data;
    it->next->next = NULL;
    return  chain;
}
struct HttpRequestResponse {
    int ssl;
    uws_res_t *res;
    uws_req_t *req;
    uws_socket_context_t *context;
    Chain* onAbort;
    Chain* onWritable;
};

struct WebSocket {
    int ssl;
    uws_websocket_t *socket;
};
struct HttpOnDataParser {
    void *data;
    struct ByteArrayList *body;
    struct HttpRequestResponse *reqRes;
    MSCHandle *handler;
};
struct HttpServer {
    int ssl;
    bool released;
    uws_app_t *app;
    struct us_socket_context_options_t options;
    struct HttpRequestBindBuffer *bindings;
};
struct WsBindings {
    MSCHandle *open;
    MSCHandle *close;
    MSCHandle *message;
    MSCHandle *subscription;
    MSCHandle *ping;
    MSCHandle *pong;
    MSCHandle *drain;
    MSCHandle *upgrade;
};
struct HttpRequestBind {
    struct HttpServer *server;
    MSCHandle *handler;
    struct WsBindings *wsHandlers;
};

struct HttpRequestBindBuffer {
    struct HttpRequestBind *value;
    struct HttpRequestBindBuffer *next;
};

static MSCHandle *httpReqResClass;
static MSCHandle *httpServerClass;
static MSCHandle *wsSocketClass;
static MSCHandle *fnCall;
static MSCHandle *fnCall1;
static MSCHandle *fnCall2;
static MSCHandle *fnCall3;
static MSCHandle *fnCall4;


static size_t byteArrayListSize(struct ByteArrayList *list) {
    size_t size = 0;
    struct ByteArrayList *item = list;
    while (item) {
        size += item->length;
        item = item->next;
    }
    return size;
}

static char *toByteArray(struct ByteArrayList *list) {
    size_t length = byteArrayListSize(list);
    char *data = malloc((length + 1) * sizeof(const char));
    struct ByteArrayList *item = list;
    const char *it = data;
    while (item) {
        memcpy(it, item->data, item->length * sizeof(const char));
        it = it + item->length;
        item = item->next;
    }
    data[length] = '\0';
    return data;
}

struct ByteArrayList *byteArrayListInit(const char *data, size_t size) {
    struct ByteArrayList *list = malloc(sizeof(struct ByteArrayList));
    list->next = NULL;
    list->length = size;
    list->data = data;
    return list;
}

struct ByteArrayList *byteArrayListAppend(struct ByteArrayList *list, const char *data, size_t size) {
    if (list == NULL) {
        return byteArrayListInit(data, size);
    }
    if (list->length == 0 && list->data == NULL) {
        list->data = data;
        list->length = size;
        list->next = NULL;
        return list;
    }
    struct ByteArrayList *it = list;
    while (!it->next) {
        it = it->next;
    }
    struct ByteArrayList *item = malloc(sizeof(struct ByteArrayList));
    item->next = NULL;
    item->data = data;
    item->length = size;
    it->next = item;
}

static void httpBindRequest(struct HttpServer *server, struct HttpRequestBind *req) {
    if (server->bindings == NULL) {
        server->bindings = (struct HttpRequestBindBuffer *) malloc(sizeof(struct HttpRequestBindBuffer));
        server->bindings->value = req;
        server->bindings->next = NULL;
        return;
    }
    struct HttpRequestBindBuffer *item = server->bindings;
    while (item->next) {
        item = item->next;
    }
    item->next = (struct HttpRequestBindBuffer *) malloc(sizeof(struct HttpRequestBindBuffer));
    item->next->value = req;
    item->next->next = NULL;
}


/*void uwsinitHttp(struct us_socket_context_options_t options) {
    httpApp = uws_create_app(SSL, options);
}*/



static void uwsOnReqAbort(uws_res_t *res, void *optional_data) {
    // struct HttpRequestResponse *reqRes = (struct HttpRequestResponse *) optional_data;
    // free(reqRes);
}

static void uwsOnData(uws_res_t *res, const char *chunk, size_t chunk_length, bool is_end, void *optional_data) {
    struct HttpOnDataParser *parser = (struct HttpOnDataParser *) optional_data;
    if (is_end) {

        parser->body = byteArrayListAppend(parser->body, chunk, chunk_length);
        Djuru *djuru = getCurrentThread();
        MSCEnsureSlots(djuru, 3);
        if (parser->body == NULL) {
            MSCSetSlotNull(djuru, 2);
        } else {
            char *bytes = toByteArray(parser->body);
            MSCSetSlotBytes(djuru, 2, bytes, strlen(bytes));
            free(bytes);
        }
        MSCHandle *handler = parser->handler;
        free(parser);
        schedulerResume(handler, true);
        schedulerFinishResume();
    } else {
        parser->body = byteArrayListAppend(parser->body, chunk, chunk_length);
    }
}

static void uwsHandler(uws_res_t *res, uws_req_t *req, void *user_data) {

    Djuru *djuru = getCurrentThread();
    struct HttpRequestBind *request = (struct HttpRequestBind *) user_data;
    MSCEnsureSlots(djuru, 2);
    // create a new request instance
    MSCSetSlotHandle(djuru, 0, request->handler);
    MSCSetSlotHandle(djuru, 1, httpReqResClass);
    struct HttpRequestResponse *reqRes = MSCSetSlotNewExtern(djuru, 1, 1, sizeof(struct HttpRequestResponse));
    reqRes->ssl = request->server->ssl;
    reqRes->res = res;
    reqRes->req = req;
    reqRes->onAbort = NULL;
    reqRes->onWritable = NULL;
    reqRes->context = NULL;

    uws_res_on_aborted(reqRes->ssl, reqRes->res, uwsOnReqAbort, reqRes);

    MSCCall(djuru, fnCall1);
}



void httpServerReqBody(Djuru *djuru) {

    struct HttpRequestResponse *reqRes = (struct HttpRequestResponse *) MSCGetSlotExtern(djuru, 0);
    struct HttpOnDataParser *parser = malloc(sizeof(struct HttpOnDataParser));

    parser->reqRes = reqRes;
    parser->body = NULL;
    parser->data = NULL;
    parser->handler = MSCGetSlotHandle(djuru, 1);
    uws_res_on_data(reqRes->ssl, reqRes->res, uwsOnData, parser);
}

void httpServerReqParam(Djuru *djuru) {
    struct HttpRequestResponse *reqRes = (struct HttpRequestResponse *) MSCGetSlotExtern(djuru, 0);
    unsigned short index = (unsigned short) MSCGetSlotDouble(djuru, 1);
    const char *value;
    size_t size = uws_req_get_parameter(reqRes->req, index, &value);
    MSCSetSlotBytes(djuru, 0, value, size);
}
void httpServerReqRemoteAddress(Djuru *djuru) {
    struct HttpRequestResponse *reqRes = (struct HttpRequestResponse *) MSCGetSlotExtern(djuru, 0);
    const char *value;
    size_t size = uws_res_get_remote_address_as_text(reqRes->ssl, reqRes->res, &value);

    MSCSetSlotBytes(djuru, 0, value, size);
}

void httpServerReqUrl(Djuru *djuru) {
    struct HttpRequestResponse *reqRes = (struct HttpRequestResponse *) MSCGetSlotExtern(djuru, 0);
    const char *value;
    size_t size = uws_req_get_url(reqRes->req, &value);
    MSCSetSlotBytes(djuru, 0, value, size);
}

void httpServerReqFullUrl(Djuru *djuru) {
    struct HttpRequestResponse *reqRes = (struct HttpRequestResponse *) MSCGetSlotExtern(djuru, 0);
    const char *value;
    size_t size = uws_req_get_full_url(reqRes->req, &value);
    MSCSetSlotBytes(djuru, 0, value, size);
}

void httpServerReqQuery(Djuru *djuru) {
    struct HttpRequestResponse *reqRes = (struct HttpRequestResponse *) MSCGetSlotExtern(djuru, 0);
    const char *value;
    int size;
    const char *key = MSCGetSlotBytes(djuru, 1, &size);
    size_t len = uws_req_get_query(reqRes->req, key, (size_t) size, &value);
    MSCSetSlotBytes(djuru, 0, value, len);
}
static void freeHandleChain(Djuru* djuru, Chain* chain) {
    Chain* it = chain;
    while (it) {
        MSCReleaseHandle(djuru, (MSCHandle*)it->data);
        Chain* tmp = it;
        it = it->next;
        free(tmp);
    }
}
void httpServerReqDestroy(void *data) {
    struct HttpRequestResponse *reqRes = (struct HttpRequestResponse *)data;
    Djuru* djuru = getCurrentThread();
    if(reqRes->onAbort) {
        freeHandleChain(djuru, reqRes->onAbort);
        reqRes->onAbort = NULL;
    }
    if(reqRes->onWritable) {
        freeHandleChain(djuru, reqRes->onWritable);
        reqRes->onWritable = NULL;
    }
}

void httpServerReqHeader(Djuru *djuru) {
    struct HttpRequestResponse *reqRes = (struct HttpRequestResponse *) MSCGetSlotExtern(djuru, 0);
    const char *value;
    int valueSize;
    const char *key = MSCGetSlotBytes(djuru, 1, &valueSize);
    size_t size = uws_req_get_header(reqRes->req, key, (size_t) valueSize, &value);
    MSCSetSlotBytes(djuru, 0, value, size);
}

static void reqForEachHeaderHandler(const char *header_name, size_t header_name_size, const char *header_value,
                                    size_t header_value_size, void *user_data) {
    Djuru *djuru = (Djuru *) user_data;
    MSCSetSlotNewMap(djuru, 1);
    // push header name
    MSCSetSlotString(djuru, 2, "name");
    MSCSetSlotBytes(djuru, 3, header_name, header_name_size);
    MSCSetMapValue(djuru, 1, 2, 3);
    // push header value
    MSCSetSlotString(djuru, 2, "value");
    MSCSetSlotBytes(djuru, 3, header_value, header_value_size);
    MSCSetMapValue(djuru, 1, 2, 3);
    MSCInsertInList(djuru, 0, -1, 1);
}

void httpServerReqHeaders(Djuru *djuru) {
    struct HttpRequestResponse *reqRes = (struct HttpRequestResponse *) MSCGetSlotExtern(djuru, 0);
    MSCSetSlotNewList(djuru, 0);
    MSCEnsureSlots(djuru, 4);
    uws_req_for_each_header(reqRes->req, reqForEachHeaderHandler, djuru);
}

void httpServerReqMethod(Djuru *djuru) {
    struct HttpRequestResponse *reqRes = (struct HttpRequestResponse *) MSCGetSlotExtern(djuru, 0);
    const char *value;
    size_t size = uws_req_get_method(reqRes->req, &value);
    MSCSetSlotBytes(djuru, 0, value, size);
}

void httpServerReqMethodCaseSensitive(Djuru *djuru) {
    struct HttpRequestResponse *reqRes = (struct HttpRequestResponse *) MSCGetSlotExtern(djuru, 0);
    const char *value;
    size_t size =  uws_req_get_case_sensitive_method(reqRes->req, &value);
    MSCSetSlotBytes(djuru, 0, value, size);
}

static void reqOnAbortHandler(uws_res_t *res, void *optional_data) {
    MSCHandle *cb = (MSCHandle *) optional_data;
    Djuru *djuru = getCurrentThread();
    MSCEnsureSlots(djuru, 1);
    MSCSetSlotHandle(djuru, 0, cb);
    MSCReleaseHandle(djuru, cb);
    MSCCall(djuru, fnCall);
}

static void reqOnDataHandler(uws_res_t *res, const char *chunk, size_t chunk_length, bool is_end, void *optional_data) {
    MSCHandle *cb = (MSCHandle *) optional_data;
    Djuru *djuru = getCurrentThread();
    MSCEnsureSlots(djuru, 3);
    MSCSetSlotHandle(djuru, 0, cb);
    MSCSetSlotBytes(djuru, 1, chunk, chunk_length);
    MSCSetSlotBool(djuru, 2, is_end);
    MSCCall(djuru, fnCall2);
    if (is_end) {
        MSCReleaseHandle(djuru, cb);
    }
}

static bool reqOnWritableHandler(uws_res_t *res, uintmax_t offset, void *optional_data) {
    MSCHandle *cb = (MSCHandle *) optional_data;
    Djuru *djuru = getCurrentThread();
    MSCEnsureSlots(djuru, 2);
    MSCSetSlotDouble(djuru, 1, offset);
    MSCSetSlotHandle(djuru, 0, cb);
    MSCInterpretResult result = MSCCall(djuru, fnCall1);
    if (result != RESULT_SUCCESS) {
        return false;
    }
    if (MSCGetSlotType(djuru, 0) != MSC_TYPE_BOOL) {
        return false;
    }
    return MSCGetSlotBool(djuru, 0);
}

void httpServerReqOnAbort(Djuru *djuru) {
    struct HttpRequestResponse *reqRes = (struct HttpRequestResponse *) MSCGetSlotExtern(djuru, 0);
    MSCHandle *cb = MSCGetSlotHandle(djuru, 1);
    reqRes->onAbort = insertInChain(reqRes->onAbort, cb);
    uws_res_on_aborted(reqRes->ssl, reqRes->res, reqOnAbortHandler, cb);
    MSCSetSlotNull(djuru, 0);
}

void httpServerReqOnData(Djuru *djuru) {
    struct HttpRequestResponse *reqRes = (struct HttpRequestResponse *) MSCGetSlotExtern(djuru, 0);
    MSCHandle *cb = MSCGetSlotHandle(djuru, 1);
    uws_res_on_data(reqRes->ssl, reqRes->res, reqOnDataHandler, cb);
    MSCSetSlotNull(djuru, 0);
}
void httpServerReqSetYield(Djuru *djuru) {
    struct HttpRequestResponse *reqRes = (struct HttpRequestResponse *) MSCGetSlotExtern(djuru, 0);
    uws_req_set_yield(reqRes->req, MSCGetSlotBool(djuru, 1));
    MSCSetSlotNull(djuru, 0);
}
void httpServerReqYield(Djuru *djuru) {
    struct HttpRequestResponse *reqRes = (struct HttpRequestResponse *) MSCGetSlotExtern(djuru, 0);
    MSCSetSlotBool(djuru, 0, uws_req_get_yield(reqRes->req));
}
void httpServerReqIsAncient(Djuru *djuru) {
    struct HttpRequestResponse *reqRes = (struct HttpRequestResponse *) MSCGetSlotExtern(djuru, 0);
    MSCSetSlotBool(djuru, 0, uws_req_is_ancient(reqRes->req));
}

void httpServerResOnWritable(Djuru *djuru) {
    struct HttpRequestResponse *reqRes = (struct HttpRequestResponse *) MSCGetSlotExtern(djuru, 0);
    MSCHandle *cb = MSCGetSlotHandle(djuru, 1);
    reqRes->onWritable = insertInChain(reqRes->onWritable, cb);
    uws_res_on_writable(reqRes->ssl, reqRes->res, reqOnWritableHandler, cb);
    MSCSetSlotNull(djuru, 0);
}

void httpServerResEnd(Djuru *djuru) {
    struct HttpRequestResponse *reqRes = (struct HttpRequestResponse *) MSCGetSlotExtern(djuru, 0);
    int size;
    const char *data = MSCGetSlotBytes(djuru, 1, &size);
    bool closeConnection = MSCGetSlotBool(djuru, 2);
    uws_res_end(reqRes->ssl, reqRes->res, data, (size_t) size, closeConnection);

    MSCSetSlotNull(djuru, 0);
}

void httpServerResEndWithoutBody(Djuru *djuru) {
    struct HttpRequestResponse *reqRes = (struct HttpRequestResponse *) MSCGetSlotExtern(djuru, 0);
    bool closeConnection = MSCGetSlotBool(djuru, 1);
    uws_res_end_without_body(reqRes->ssl, reqRes->res, closeConnection);
    MSCSetSlotNull(djuru, 0);
}

void httpServerResPause(Djuru *djuru) {
    struct HttpRequestResponse *reqRes = (struct HttpRequestResponse *) MSCGetSlotExtern(djuru, 0);
    uws_res_pause(reqRes->ssl, reqRes->res);
    MSCSetSlotNull(djuru, 0);
}

void httpServerResResume(Djuru *djuru) {
    struct HttpRequestResponse *reqRes = (struct HttpRequestResponse *) MSCGetSlotExtern(djuru, 0);
    uws_res_resume(reqRes->ssl, reqRes->res);
    MSCSetSlotNull(djuru, 0);
}

void httpServerResWrite(Djuru *djuru) {
    struct HttpRequestResponse *reqRes = (struct HttpRequestResponse *) MSCGetSlotExtern(djuru, 0);
    int size;
    const char *data = MSCGetSlotBytes(djuru, 1, &size);
    uws_res_write(reqRes->ssl, reqRes->res, data, (size_t) size);
    MSCSetSlotNull(djuru, 0);
}

void httpServerResWriteContinue(Djuru *djuru) {
    struct HttpRequestResponse *reqRes = (struct HttpRequestResponse *) MSCGetSlotExtern(djuru, 0);
    uws_res_write_continue(reqRes->ssl, reqRes->res);
    MSCSetSlotNull(djuru, 0);
}

void httpServerResWriteStatus(Djuru *djuru) {
    struct HttpRequestResponse *reqRes = (struct HttpRequestResponse *) MSCGetSlotExtern(djuru, 0);
    int size;

    const char *status = MSCGetSlotBytes(djuru, 1, &size);
    uws_res_write_status(reqRes->ssl, reqRes->res, status, (size_t) size);
    MSCSetSlotNull(djuru, 0);
}

void httpServerResWriteHeader(Djuru *djuru) {
    struct HttpRequestResponse *reqRes = (struct HttpRequestResponse *) MSCGetSlotExtern(djuru, 0);
    int keySize;
    const char *key = MSCGetSlotBytes(djuru, 1, &keySize);
    int valueSize;
    const char *value = MSCGetSlotBytes(djuru, 2, &valueSize);
    uws_res_write_header(reqRes->ssl, reqRes->res, key, (size_t) keySize, value, (size_t) valueSize);
    MSCSetSlotNull(djuru, 0);
}

void httpServerResWriteHeaderInt(Djuru *djuru) {
    struct HttpRequestResponse *reqRes = (struct HttpRequestResponse *) MSCGetSlotExtern(djuru, 0);
    int keySize;
    const char *key = MSCGetSlotBytes(djuru, 1, &keySize);
    uint64_t value = (uint64_t) MSCGetSlotDouble(djuru, 2);
    uws_res_write_header_int(reqRes->ssl, reqRes->res, key, (size_t) keySize, value);
    MSCSetSlotNull(djuru, 0);
}


void httpServerResHasResponded(Djuru *djuru) {
    struct HttpRequestResponse *reqRes = (struct HttpRequestResponse *) MSCGetSlotExtern(djuru, 0);
    MSCSetSlotBool(djuru, 0, uws_res_has_responded(reqRes->ssl, reqRes->res));
}


void httpServerResTryEnd(Djuru *djuru) {
    struct HttpRequestResponse *reqRes = (struct HttpRequestResponse *) MSCGetSlotExtern(djuru, 0);
    int size;
    const char *data = MSCGetSlotBytes(djuru, 1, &size);
    uintmax_t total = (uintmax_t) MSCGetSlotDouble(djuru, 2);
    bool closeConnection = MSCGetSlotBool(djuru, 3);
    uws_try_end_result_t resut = uws_res_try_end(reqRes->ssl, reqRes->res, data, (size_t) size, total, closeConnection);

    MSCSetSlotBool(djuru, 0, resut.ok);
}

static void uwsCorkRes(uws_res_t *res, void *userData) {
    struct HttpRequestResponse *reqRes = (struct HttpRequestResponse *) userData;
    Djuru *djuru = getCurrentThread();
    // update reqRes with corked res

    MSCSetSlotHandle(djuru, 0, httpReqResClass);
    struct HttpRequestResponse *newReqRes = MSCSetSlotNewExtern(djuru, 0, 0, sizeof(struct HttpRequestResponse));
    newReqRes->ssl = reqRes->ssl;
    newReqRes->res = res;
    newReqRes->req = reqRes->req;
    newReqRes->context = reqRes->context;
    newReqRes->onWritable = NULL;
    newReqRes->onAbort = NULL;
}

static struct HttpRequestBind *createRequest(Djuru *djuru) {
    struct HttpServer *server = (struct HttpServer *) MSCGetSlotExtern(djuru, 0);
    struct HttpRequestBind *request = malloc(sizeof(struct HttpRequestBind));
    request->handler = MSCGetSlotHandle(djuru, 2);
    request->server = server;
    request->wsHandlers = NULL;
    // add to bind list
    httpBindRequest(server, request);
    return request;
}

static struct HttpRequestBind *createWsRequest(Djuru *djuru) {
    struct HttpServer *server = (struct HttpServer *) MSCGetSlotExtern(djuru, 0);
    struct HttpRequestBind *request = malloc(sizeof(struct HttpRequestBind));
    request->handler = NULL;
    request->server = server;
    request->wsHandlers = malloc(sizeof(struct WsBindings));
    request->wsHandlers->subscription = NULL;
    request->wsHandlers->open = NULL;
    request->wsHandlers->close = NULL;
    request->wsHandlers->message = NULL;
    request->wsHandlers->ping = NULL;
    request->wsHandlers->pong = NULL;
    request->wsHandlers->drain = NULL;
    request->wsHandlers->upgrade = NULL;

    if (MSCGetSlotType(djuru, 2) == MSC_TYPE_MAP) {
        MSCSetSlotString(djuru, 3, "upgrade");
        MSCGetMapValue(djuru, 2, 3, 3);
        if (MSCGetSlotType(djuru, 3) != MSC_TYPE_NULL) {
            request->wsHandlers->upgrade = MSCGetSlotHandle(djuru, 3);
        }
        MSCSetSlotString(djuru, 3, "open");
        MSCGetMapValue(djuru, 2, 3, 3);
        if (MSCGetSlotType(djuru, 3) != MSC_TYPE_NULL) {
            request->wsHandlers->open = MSCGetSlotHandle(djuru, 3);
        }
        MSCSetSlotString(djuru, 3, "close");
        MSCGetMapValue(djuru, 2, 3, 3);
        if (MSCGetSlotType(djuru, 3) != MSC_TYPE_NULL) {
            request->wsHandlers->close = MSCGetSlotHandle(djuru, 3);
        }
        MSCSetSlotString(djuru, 3, "drain");
        MSCGetMapValue(djuru, 2, 3, 3);
        if (MSCGetSlotType(djuru, 3) != MSC_TYPE_NULL) {
            request->wsHandlers->drain = MSCGetSlotHandle(djuru, 3);
        }
        MSCSetSlotString(djuru, 3, "ping");
        MSCGetMapValue(djuru, 2, 3, 3);
        if (MSCGetSlotType(djuru, 3) != MSC_TYPE_NULL) {
            request->wsHandlers->ping = MSCGetSlotHandle(djuru, 3);
        }
        MSCSetSlotString(djuru, 3, "pong");
        MSCGetMapValue(djuru, 2, 3, 3);
        if (MSCGetSlotType(djuru, 3) != MSC_TYPE_NULL) {
            request->wsHandlers->pong = MSCGetSlotHandle(djuru, 3);
        }
        MSCSetSlotString(djuru, 3, "message");
        MSCGetMapValue(djuru, 2, 3, 3);
        if (MSCGetSlotType(djuru, 3) != MSC_TYPE_NULL) {
            request->wsHandlers->message = MSCGetSlotHandle(djuru, 3);
        }
        MSCSetSlotString(djuru, 3, "subscription");
        MSCGetMapValue(djuru, 2, 3, 3);
        if (MSCGetSlotType(djuru, 3) != MSC_TYPE_NULL) {
            request->wsHandlers->subscription = MSCGetSlotHandle(djuru, 3);
        }
    }

    // add to bind list
    httpBindRequest(server, request);
    return request;
}

void httpServerResCork(Djuru *djuru) {
    struct HttpRequestResponse *reqRes = (struct HttpRequestResponse *) MSCGetSlotExtern(djuru, 0);
    uws_res_cork(reqRes->ssl, reqRes->res, uwsCorkRes, reqRes);
}

void ensureHttpComponentsInit(Djuru *djuru) {
    MSCGetVariable(djuru, "net", "HttpReqRes", 0);
    httpReqResClass = MSCGetSlotHandle(djuru, 0);
    MSCGetVariable(djuru, "net", "HttpServer_", 0);
    httpServerClass = MSCGetSlotHandle(djuru, 0);
    MSCGetVariable(djuru, "net", "WebSocket", 0);
    wsSocketClass = MSCGetSlotHandle(djuru, 0);
    MVM* vm = djuru->vm;
    fnCall = MSCMakeCallHandle(vm, "weele()");
    fnCall1 = MSCMakeCallHandle(vm, "weele(_)");
    fnCall2 = MSCMakeCallHandle(vm, "weele(_,_)");
    fnCall3 = MSCMakeCallHandle(vm, "weele(_,_,_)");
    fnCall4 = MSCMakeCallHandle(vm, "weele(_,_,_,_)");

}

static void releaseApp(struct HttpServer *server) {
    if (server == NULL || server->released) {
        return;
    }
    server->released = true;
    struct HttpRequestBindBuffer *bindings = server->bindings;
    Djuru *djuru = getCurrentThread();
    while (bindings) {
        if (bindings->value->handler != NULL) {
            MSCReleaseHandle(djuru, bindings->value->handler);
        }
        if (bindings->value->wsHandlers) {
            struct WsBindings *wsBindings = bindings->value->wsHandlers;
            if (wsBindings->drain) MSCReleaseHandle(djuru, wsBindings->drain);
            if (wsBindings->open) MSCReleaseHandle(djuru, wsBindings->open);
            if (wsBindings->close) MSCReleaseHandle(djuru, wsBindings->close);
            if (wsBindings->message) MSCReleaseHandle(djuru, wsBindings->message);
            if (wsBindings->upgrade) MSCReleaseHandle(djuru, wsBindings->upgrade);
            if (wsBindings->ping) MSCReleaseHandle(djuru, wsBindings->ping);
            if (wsBindings->pong) MSCReleaseHandle(djuru, wsBindings->pong);
            if (wsBindings->subscription) MSCReleaseHandle(djuru, wsBindings->subscription);
            free(bindings->value->wsHandlers);
        }
        struct HttpRequestBindBuffer *old = bindings;
        bindings = bindings->next;
        free(old->value);
        free(old);
    }
}

void httpServerDestroy(void *data) {
    struct HttpServer *server = (struct HttpServer *) data;
    releaseApp(server);
    uws_app_destroy(server->ssl, server->app);
}

void httpServerInit(Djuru *djuru) {
    // get options
    int ssl = (int) MSCGetSlotBool(djuru, 1);

    const char *cert_file = NULL;//MSCGetSlotString(djuru, 2);// cert file
    const char *key_file = NULL;//MSCGetSlotString(djuru, 3);// key file
    const char *passphrase = NULL;//MSCGetSlotString(djuru, 4);// passphrase
    const char *ca_file_name = NULL;//MSCGetSlotString(djuru, 5);// ca_file_name
    const char *dh_params_file_name = NULL;//MSCGetSlotString(djuru, 6);// dh_params_file_name
    const char *ssl_ciphers = NULL; //MSCGetSlotString(djuru, 7);// ssl_ciphers
    bool ssl_prefer_low_memory_usage = false; // MSCGetSlotBool(djuru, 8);// ssl_prefer_low_memory_usage
    MSCSetSlotHandle(djuru, 0, httpServerClass);
    struct HttpServer *httpServer = (struct HttpServer *) MSCSetSlotNewExtern(djuru, 0, 0, sizeof(struct HttpServer));
    httpServer->options = (struct us_socket_context_options_t) {
            .key_file_name = key_file,
            .cert_file_name = cert_file,
            .passphrase = passphrase,
            .ca_file_name = ca_file_name,
            .ssl_prefer_low_memory_usage = ssl_prefer_low_memory_usage,
            .dh_params_file_name = dh_params_file_name,
            .ssl_ciphers = ssl_ciphers
    };
    // httpServer->bindings = (struct HttpRequestBindBuffer*)malloc(sizeof(struct HttpRequestBindBuffer));
    httpServer->ssl = ssl;
    uws_app_t *app = uws_create_app(ssl, httpServer->options);
    httpServer->app = app;
    httpServer->released = false;
    // uwsinitHttp(options);
}
// attach endpoint to http method
// nin app = Http()
// app.get("") {(req, res) =>
//
// }
// app.post("", data) {(req, res) =>
// }

void httpServerGetMethod(Djuru *djuru) {
    const char *pattern = MSCGetSlotString(djuru, 1);
    struct HttpRequestBind *request = createRequest(djuru);
    uws_app_get(request->server->ssl, request->server->app, pattern, uwsHandler, request);
}


void httpServerPatchMethod(Djuru *djuru) {
    const char *pattern = MSCGetSlotString(djuru, 1);
    struct HttpRequestBind *request = createRequest(djuru);
    uws_app_patch(request->server->ssl, request->server->app, pattern, uwsHandler, request);
}

void httpServerPostMethod(Djuru *djuru) {
    const char *pattern = MSCGetSlotString(djuru, 1);
    struct HttpRequestBind *request = createRequest(djuru);
    uws_app_post(request->server->ssl, request->server->app, pattern, uwsHandler, request);
}

void httpServerPutMethod(Djuru *djuru) {
    const char *pattern = MSCGetSlotString(djuru, 1);
    struct HttpRequestBind *request = createRequest(djuru);
    uws_app_put(request->server->ssl, request->server->app, pattern, uwsHandler, request);
}

void httpServerOptionsMethod(Djuru *djuru) {
    const char *pattern = MSCGetSlotString(djuru, 1);
    struct HttpRequestBind *request = createRequest(djuru);
    uws_app_options(request->server->ssl, request->server->app, pattern, uwsHandler, request);
}

void httpServerDeleteMethod(Djuru *djuru) {
    const char *pattern = MSCGetSlotString(djuru, 1);
    struct HttpRequestBind *request = createRequest(djuru);
    uws_app_delete(request->server->ssl, request->server->app, pattern, uwsHandler, request);
}

void httpServerAnyMethod(Djuru *djuru) {
    const char *pattern = MSCGetSlotString(djuru, 1);
    struct HttpRequestBind *request = createRequest(djuru);
    uws_app_any(request->server->ssl, request->server->app, pattern, uwsHandler, request);
}
void httpServerHeadMethod(Djuru *djuru) {
    const char *pattern = MSCGetSlotString(djuru, 1);
    struct HttpRequestBind *request = createRequest(djuru);
    uws_app_head(request->server->ssl, request->server->app, pattern, uwsHandler, request);
}
void httpServerTraceMethod(Djuru *djuru) {
    const char *pattern = MSCGetSlotString(djuru, 1);
    struct HttpRequestBind *request = createRequest(djuru);
    uws_app_trace(request->server->ssl, request->server->app, pattern, uwsHandler, request);
}

void httpServerConnectMethod(Djuru *djuru) {
    const char *pattern = MSCGetSlotString(djuru, 1);
    struct HttpRequestBind *request = createRequest(djuru);
    uws_app_connect(request->server->ssl, request->server->app, pattern, uwsHandler, request);
}

static struct WebSocket *createWebSocket(Djuru *djuru, int slot, uws_websocket_t *ws, int ssl) {
    MSCSetSlotHandle(djuru, slot, wsSocketClass);
    struct WebSocket *data = (struct WebSocket *) MSCSetSlotNewExtern(djuru, slot, slot, sizeof(struct WebSocket));
    data->ssl = ssl;
    data->socket = ws;
}

static void wsOnCloseHandler(uws_websocket_t *ws, int code, const char *message, size_t length, void *user_data) {
    printf("wsOnCloseHandler:: %.*s, code: %d\n", (int)length, message, code);
    struct HttpRequestBind *handle = (struct HttpRequestBind *) user_data;
    if(!handle->wsHandlers->close) {
        return;
    }
    Djuru *djuru = getCurrentThread();
    MSCEnsureSlots(djuru, 4);
    MSCSetSlotHandle(djuru, 0, handle->wsHandlers->close);
    createWebSocket(djuru, 1, ws, handle->server->ssl);
    MSCSetSlotBytes(djuru, 2, message, length);
    MSCSetSlotDouble(djuru, 3, code);
    MSCCall(djuru, fnCall3);
}

static void wsOnOpenHandler(uws_websocket_t *ws, void *user_data) {
    printf("wsOnOpenHandler::\n");
    struct HttpRequestBind *handle = (struct HttpRequestBind *) user_data;
    if(!handle->wsHandlers->open) {
        return;
    }
    Djuru *djuru = getCurrentThread();
    MSCEnsureSlots(djuru, 2);
    MSCSetSlotHandle(djuru, 0, handle->wsHandlers->open);
    createWebSocket(djuru, 1, ws, handle->server->ssl);
    MSCCall(djuru, fnCall1);
}

static void wsOnDrainHandler(uws_websocket_t *ws, void *user_data) {
    printf("wsOnDrainHandler::\n");
    struct HttpRequestBind *handle = (struct HttpRequestBind *) user_data;
    if(!handle->wsHandlers->drain) {
        return;
    }
    Djuru *djuru = getCurrentThread();
    MSCEnsureSlots(djuru, 2);
    MSCSetSlotHandle(djuru, 0, handle->wsHandlers->drain);
    createWebSocket(djuru, 1, ws, handle->server->ssl);
    MSCCall(djuru, fnCall1);
}

static void
wsOnMessageHandler(uws_websocket_t *ws, const char *message, size_t length, uws_opcode_t opcode, void *user_data) {
    printf("wsOnMessageHandler::\n");
    struct HttpRequestBind *handle = (struct HttpRequestBind *) user_data;
    if(!handle->wsHandlers->message) {
        return;
    }
    Djuru *djuru = getCurrentThread();
    MSCEnsureSlots(djuru, 4);
    MSCSetSlotHandle(djuru, 0, handle->wsHandlers->message);
    createWebSocket(djuru, 1, ws, handle->server->ssl);
    MSCSetSlotBytes(djuru, 2, message, length);
    MSCSetSlotDouble(djuru, 3, opcode);
    MSCCall(djuru, fnCall3);
}

static void wsPongHandler(uws_websocket_t *ws, const char *message, size_t length, void *user_data) {
    printf("wsPongHandler::\n");
    struct HttpRequestBind *handle = (struct HttpRequestBind *) user_data;
    if(!handle->wsHandlers->pong) {
        return;
    }
    Djuru *djuru = getCurrentThread();
    MSCEnsureSlots(djuru, 3);
    MSCSetSlotHandle(djuru, 0, handle->wsHandlers->pong);
    createWebSocket(djuru, 1, ws, handle->server->ssl);
    MSCSetSlotBytes(djuru, 2, message, length);
    MSCCall(djuru, fnCall2);
}

static void wsPingHandler(uws_websocket_t *ws, const char *message, size_t length, void *user_data) {
    printf("wsPingHandler::\n");
    struct HttpRequestBind *handle = (struct HttpRequestBind *) user_data;
    if(!handle->wsHandlers->ping) {
        return;
    }
    Djuru *djuru = getCurrentThread();
    MSCEnsureSlots(djuru, 3);
    MSCSetSlotHandle(djuru, 0, handle->wsHandlers->ping);
    createWebSocket(djuru, 1, ws, handle->server->ssl);
    MSCSetSlotBytes(djuru, 2, message, length);
    MSCCall(djuru, fnCall2);
}

static void wsUpgradeHandler(uws_res_t *res, uws_req_t *req, uws_socket_context_t *context, void *user_data) {
    printf("wsUpgradeHandler::\n");
    struct HttpRequestBind *handle = (struct HttpRequestBind *) user_data;
    uws_res_on_aborted(handle->server->ssl, res, uwsOnReqAbort, NULL);
    if(!handle->wsHandlers->upgrade) {
        return;
    }
    Djuru *djuru = getCurrentThread();
    MSCEnsureSlots(djuru, 2);
    // create a new reqres instance
    MSCSetSlotHandle(djuru, 0, handle->wsHandlers->upgrade);
    MSCSetSlotHandle(djuru, 1, httpReqResClass);
    struct HttpRequestResponse *reqRes = MSCSetSlotNewExtern(djuru, 1, 1, sizeof(struct HttpRequestResponse));
    // (struct HttpRequestResponse *) MSCGetSlotExtern(djuru, 2);
    *reqRes = (struct HttpRequestResponse) {.req = req, .res = res, .ssl = handle->server->ssl, .context = context};
    uws_res_on_aborted(reqRes->ssl, reqRes->res, uwsOnReqAbort, reqRes);
    MSCCall(djuru, fnCall1);
}

static void wsSubscriptionHandler(uws_websocket_t *ws, const char *topic_name, size_t topic_name_length,
                                  int new_number_of_subscriber, int old_number_of_subscriber, void *user_data) {
    printf("wsSubscriptionHandler::\n");
    struct HttpRequestBind *handle = (struct HttpRequestBind *) user_data;
    if(!handle->wsHandlers->subscription) {
        return;
    }
    Djuru *djuru = getCurrentThread();
    MSCEnsureSlots(djuru, 5);
    MSCSetSlotHandle(djuru, 0, handle->wsHandlers->subscription);
    createWebSocket(djuru, 1, ws, handle->server->ssl);
    MSCSetSlotBytes(djuru, 2, topic_name, topic_name_length);
    MSCSetSlotDouble(djuru, 3, new_number_of_subscriber);
    MSCSetSlotDouble(djuru, 4, old_number_of_subscriber);

    MSCCall(djuru, fnCall4);
}

void wsEnd(Djuru *djuru) {
    struct WebSocket *socket = (struct WebSocket *) MSCGetSlotExtern(djuru, 0);
    int size;
    const char *message = MSCGetSlotBytes(djuru, 1, &size);
    uws_ws_end(socket->ssl, socket->socket, (int) MSCGetSlotDouble(djuru, 2), message, (size_t) size);
}

static void wsCorkHandler(void *userData) {

}

void wsCork(Djuru *djuru) {
    struct WebSocket *socket = (struct WebSocket *) MSCGetSlotExtern(djuru, 0);
    uws_ws_cork(socket->ssl, socket->socket, wsCorkHandler, NULL);
}

void wsClose(Djuru *djuru) {
    struct WebSocket *socket = (struct WebSocket *) MSCGetSlotExtern(djuru, 0);
    uws_ws_close(socket->ssl, socket->socket);
}

void wsRemoteAddress(Djuru *djuru) {
    struct WebSocket *socket = (struct WebSocket *) MSCGetSlotExtern(djuru, 0);
    const char *dest;
    size_t size = uws_ws_get_remote_address_as_text(socket->ssl, socket->socket, &dest);
    MSCSetSlotBytes(djuru, 0, dest, size);

}

void wsPublish(Djuru *djuru) {
    struct WebSocket *socket = (struct WebSocket *) MSCGetSlotExtern(djuru, 0);
    int topicSize;
    const char *topic = MSCGetSlotBytes(djuru, 1, &topicSize);
    int messageSize;
    const char *message = MSCGetSlotBytes(djuru, 2, &messageSize);
    bool ok = uws_ws_publish(socket->ssl, socket->socket, topic, (size_t) topicSize, message, (size_t) messageSize);
    MSCSetSlotBool(djuru, 0, ok);
}

void wsPublishWithOptions(Djuru *djuru) {
    struct WebSocket *socket = (struct WebSocket *) MSCGetSlotExtern(djuru, 0);
    int topicSize;
    const char *topic = MSCGetSlotBytes(djuru, 1, &topicSize);
    int messageSize;
    const char *message = MSCGetSlotBytes(djuru, 2, &messageSize);
    bool ok = uws_ws_publish_with_options(socket->ssl, socket->socket, topic, (size_t) topicSize, message,
                                          (size_t) messageSize, (uws_opcode_t) (int) MSCGetSlotDouble(djuru, 3),
                                          MSCGetSlotBool(djuru, 4));
    MSCSetSlotBool(djuru, 0, ok);
}

void wsSend(Djuru *djuru) {
    struct WebSocket *socket = (struct WebSocket *) MSCGetSlotExtern(djuru, 0);
    int messageSize;
    const char *message = MSCGetSlotBytes(djuru, 1, &messageSize);
    uws_sendstatus_t ok = uws_ws_send(socket->ssl, socket->socket, message, (size_t) messageSize,
                                      (uws_opcode_t) (int) MSCGetSlotDouble(djuru, 2));
    MSCSetSlotDouble(djuru, 0, ok);
}

void wsSubscribe(Djuru *djuru) {
    struct WebSocket *socket = (struct WebSocket *) MSCGetSlotExtern(djuru, 0);
    int topicSize;
    const char *topic = MSCGetSlotBytes(djuru, 1, &topicSize);
    bool ok = uws_ws_subscribe(socket->ssl, socket->socket, topic, (size_t) topicSize);
    MSCSetSlotBool(djuru, 0, ok);
}

void wsUnsubscribe(Djuru *djuru) {
    struct WebSocket *socket = (struct WebSocket *) MSCGetSlotExtern(djuru, 0);
    int topicSize;
    const char *topic = MSCGetSlotBytes(djuru, 1, &topicSize);
    bool ok = uws_ws_unsubscribe(socket->ssl, socket->socket, topic, (size_t) topicSize);
    MSCSetSlotBool(djuru, 0, ok);
}

static void wsTopicForEachHandler(const char *name, size_t size, void *userData) {
    Djuru *djuru = (Djuru *) userData;
    // push name
    MSCSetSlotBytes(djuru, 1, name, size);
    MSCSetListElement(djuru, 0, -1, 2);
}

void wsTopicForEach(Djuru *djuru) {
    struct WebSocket *socket = (struct WebSocket *) MSCGetSlotExtern(djuru, 0);
    MSCEnsureSlots(djuru, 2);
    MSCSetSlotNewList(djuru, 0);
    uws_ws_iterate_topics(socket->ssl, socket->socket, wsTopicForEachHandler, djuru);
}

void wsBufferedAmount(Djuru *djuru) {
    struct WebSocket *socket = (struct WebSocket *) MSCGetSlotExtern(djuru, 0);
    MSCEnsureSlots(djuru, 2);
    unsigned int amount = uws_ws_get_buffered_amount(socket->ssl, socket->socket);
    MSCSetSlotDouble(djuru, 0, amount);
}

void httpServerWsMethod(Djuru *djuru) {
    const char *pattern = MSCGetSlotString(djuru, 1);
    MSCEnsureSlots(djuru, 4);
    struct HttpRequestBind *request = createWsRequest(djuru);

    uws_socket_behavior_t behavior = {
    };
    if(request->wsHandlers->subscription) {
        behavior.subscription = wsSubscriptionHandler;
    }
    if(request->wsHandlers->upgrade) {
        behavior.upgrade = wsUpgradeHandler;
    }
    if(request->wsHandlers->ping) {
        behavior.ping = wsPingHandler;
    }
    if(request->wsHandlers->pong) {
        behavior.pong = wsPongHandler;
    }
    if(request->wsHandlers->close) {
        behavior.close = wsOnCloseHandler;
    }
    if(request->wsHandlers->open) {
        behavior.open = wsOnOpenHandler;
    }
    if(request->wsHandlers->message) {
        behavior.message = wsOnMessageHandler;
    }
    if(request->wsHandlers->drain) {
        behavior.drain = wsOnDrainHandler;
    }

    if (MSCGetSlotType(djuru, 2) == MSC_TYPE_MAP) {
        MSCSetSlotString(djuru, 3, "idleTimeout");
        MSCGetMapValue(djuru, 2, 3, 3);
        if (MSCGetSlotType(djuru, 3) == MSC_TYPE_NUM) {
            behavior.idleTimeout = (unsigned short) MSCGetSlotDouble(djuru, 3);
        }
        MSCSetSlotString(djuru, 3, "maxBackpressure");
        MSCGetMapValue(djuru, 2, 3, 3);
        if (MSCGetSlotType(djuru, 3) == MSC_TYPE_NUM) {
            behavior.maxBackpressure = (unsigned int) MSCGetSlotDouble(djuru, 3);
        }
        MSCSetSlotString(djuru, 3, "maxLifetime");
        MSCGetMapValue(djuru, 2, 3, 3);
        if (MSCGetSlotType(djuru, 3) == MSC_TYPE_NUM) {
            behavior.maxLifetime = (unsigned short) MSCGetSlotDouble(djuru, 3);
        }
        MSCSetSlotString(djuru, 3, "maxPayloadLength");
        MSCGetMapValue(djuru, 2, 3, 3);
        if (MSCGetSlotType(djuru, 3) == MSC_TYPE_NUM) {
            behavior.maxPayloadLength = (unsigned int) MSCGetSlotDouble(djuru, 3);
        }
        MSCSetSlotString(djuru, 3, "resetIdleTimeoutOnSend");
        MSCGetMapValue(djuru, 2, 3, 3);
        if (MSCGetSlotType(djuru, 3) == MSC_TYPE_BOOL) {
            behavior.resetIdleTimeoutOnSend = MSCGetSlotBool(djuru, 3);
        }
        MSCSetSlotString(djuru, 3, "sendPingsAutomatically");
        MSCGetMapValue(djuru, 2, 3, 3);
        if (MSCGetSlotType(djuru, 3) == MSC_TYPE_BOOL) {
            behavior.sendPingsAutomatically = MSCGetSlotBool(djuru, 3);
        }
        MSCSetSlotString(djuru, 3, "closeOnBackpressureLimit");
        MSCGetMapValue(djuru, 2, 3, 3);
        if (MSCGetSlotType(djuru, 3) == MSC_TYPE_BOOL) {
            behavior.closeOnBackpressureLimit = MSCGetSlotBool(djuru, 3);
        }
        MSCSetSlotString(djuru, 3, "compression");
        MSCGetMapValue(djuru, 2, 3, 3);
        if (MSCGetSlotType(djuru, 3) == MSC_TYPE_BOOL) {
            behavior.compression = (uws_compress_options_t) (int) MSCGetSlotDouble(djuru, 3);
        }
    }

    uws_ws(request->server->ssl, request->server->app, pattern, behavior, request);
}

static void
uwsListenHandler(struct us_listen_socket_t *listen_socket, uws_app_listen_config_t config, void *user_data) {
    Djuru *djuru = getCurrentThread();
    if (listen_socket) {
        MSCSetSlotBool(djuru, 0, true);
    } else {
        MSCSetSlotString(djuru, 0, "Failed to listen");
        MSCAbortDjuru(djuru, 0);
    }

}

void httpServerListen(Djuru *djuru) {
    int port = (int) MSCGetSlotDouble(djuru, 1);
    struct HttpServer *server = (struct HttpServer *) MSCGetSlotExtern(djuru, 0);
    uws_app_listen(server->ssl, server->app, port, uwsListenHandler, NULL);
}

void httpServerStop(Djuru *djuru) {
    struct HttpServer *server = (struct HttpServer *) MSCGetSlotExtern(djuru, 0);
    releaseApp(server);
    uws_app_close(server->ssl, server->app);
}

void httpServerPublish(Djuru *djuru) {
    struct HttpServer *server = (struct HttpServer *) MSCGetSlotExtern(djuru, 0);
    int topicLength;
    int messageLength;
    const char* topic = MSCGetSlotBytes(djuru, 1, &topicLength);
    const char* message = MSCGetSlotBytes(djuru, 2, &messageLength);
    MSCSetSlotBool(djuru, 0, uws_publish(server->ssl, server->app, topic, (size_t) topicLength, message, (size_t) messageLength, (uws_opcode_t)(int)MSCGetSlotDouble(djuru, 3), MSCGetSlotBool(djuru, 4)));
}

void httpServerNumSubscriber(Djuru *djuru) {
    struct HttpServer *server = (struct HttpServer *) MSCGetSlotExtern(djuru, 0);
    int topicLength;
    const char* topic = MSCGetSlotBytes(djuru, 1, &topicLength);
    unsigned int res = uws_num_subscribers(server->ssl, server->app, topic, (size_t) topicLength);
    MSCSetSlotDouble(djuru, 0, res);
}
void httpServerRun(Djuru *djuru) {
    // uws_app_run(SSL, httpApp);
    // us_loop_run(uws_get_loop_with_native(getLoop()));
}

void httpShutdown() {

    Djuru *djuru = getCurrentThread();
    if (httpReqResClass) {
        MSCReleaseHandle(djuru, httpReqResClass);
    }
    if (httpServerClass) {
        MSCReleaseHandle(djuru, httpServerClass);
    }
    if (fnCall) {
        MSCReleaseHandle(djuru, fnCall);
    }
    if (fnCall1) {
        MSCReleaseHandle(djuru, fnCall1);
    }
    if (fnCall2) {
        MSCReleaseHandle(djuru, fnCall2);
    }
    if (fnCall3) {
        MSCReleaseHandle(djuru, fnCall3);
    }
    if (fnCall4) {
        MSCReleaseHandle(djuru, fnCall4);
    }
    if (wsSocketClass) {
        MSCReleaseHandle(djuru, wsSocketClass);
    }
}