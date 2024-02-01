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
        MVM *vm = getVM();
        MSCEnsureSlots(vm, 3);
        if (parser->body == NULL) {
            MSCSetSlotNull(vm, 2);
        } else {
            char *bytes = toByteArray(parser->body);
            MSCSetSlotBytes(vm, 2, bytes, strlen(bytes));
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

    MVM *vm = getVM();
    struct HttpRequestBind *request = (struct HttpRequestBind *) user_data;
    MSCEnsureSlots(vm, 2);
    // create a new request instance
    MSCSetSlotHandle(vm, 0, request->handler);
    MSCSetSlotHandle(vm, 1, httpReqResClass);
    struct HttpRequestResponse *reqRes = MSCSetSlotNewExtern(vm, 1, 1, sizeof(struct HttpRequestResponse));
    reqRes->ssl = request->server->ssl;
    reqRes->res = res;
    reqRes->req = req;
    reqRes->onAbort = NULL;
    reqRes->onWritable = NULL;
    reqRes->context = NULL;

    uws_res_on_aborted(reqRes->ssl, reqRes->res, uwsOnReqAbort, reqRes);

    MSCCall(vm, fnCall1);
}



void httpServerReqBody(MVM *vm) {

    struct HttpRequestResponse *reqRes = (struct HttpRequestResponse *) MSCGetSlotExtern(vm, 0);
    struct HttpOnDataParser *parser = malloc(sizeof(struct HttpOnDataParser));

    parser->reqRes = reqRes;
    parser->body = NULL;
    parser->data = NULL;
    parser->handler = MSCGetSlotHandle(vm, 1);
    uws_res_on_data(reqRes->ssl, reqRes->res, uwsOnData, parser);
}

void httpServerReqParam(MVM *vm) {
    struct HttpRequestResponse *reqRes = (struct HttpRequestResponse *) MSCGetSlotExtern(vm, 0);
    unsigned short index = (unsigned short) MSCGetSlotDouble(vm, 1);
    const char *value;
    size_t size = uws_req_get_parameter(reqRes->req, index, &value);
    MSCSetSlotBytes(vm, 0, value, size);
}
void httpServerReqRemoteAddress(MVM *vm) {
    struct HttpRequestResponse *reqRes = (struct HttpRequestResponse *) MSCGetSlotExtern(vm, 0);
    const char *value;
    size_t size = uws_res_get_remote_address_as_text(reqRes->ssl, reqRes->res, &value);

    MSCSetSlotBytes(vm, 0, value, size);
}

void httpServerReqUrl(MVM *vm) {
    struct HttpRequestResponse *reqRes = (struct HttpRequestResponse *) MSCGetSlotExtern(vm, 0);
    const char *value;
    size_t size = uws_req_get_url(reqRes->req, &value);
    MSCSetSlotBytes(vm, 0, value, size);
}

void httpServerReqFullUrl(MVM *vm) {
    struct HttpRequestResponse *reqRes = (struct HttpRequestResponse *) MSCGetSlotExtern(vm, 0);
    const char *value;
    size_t size = uws_req_get_full_url(reqRes->req, &value);
    MSCSetSlotBytes(vm, 0, value, size);
}

void httpServerReqQuery(MVM *vm) {
    struct HttpRequestResponse *reqRes = (struct HttpRequestResponse *) MSCGetSlotExtern(vm, 0);
    const char *value;
    int size;
    const char *key = MSCGetSlotBytes(vm, 1, &size);
    size_t len = uws_req_get_query(reqRes->req, key, (size_t) size, &value);
    MSCSetSlotBytes(vm, 0, value, len);
}
static void freeHandleChain(MVM* vm, Chain* chain) {
    Chain* it = chain;
    while (it) {
        MSCReleaseHandle(vm, (MSCHandle*)it->data);
        Chain* tmp = it;
        it = it->next;
        free(tmp);
    }
}
void httpServerReqDestroy(void *data) {
    struct HttpRequestResponse *reqRes = (struct HttpRequestResponse *)data;
    MVM* vm = getVM();
    if(reqRes->onAbort) {
        freeHandleChain(vm, reqRes->onAbort);
        reqRes->onAbort = NULL;
    }
    if(reqRes->onWritable) {
        freeHandleChain(vm, reqRes->onWritable);
        reqRes->onWritable = NULL;
    }
}

void httpServerReqHeader(MVM *vm) {
    struct HttpRequestResponse *reqRes = (struct HttpRequestResponse *) MSCGetSlotExtern(vm, 0);
    const char *value;
    int valueSize;
    const char *key = MSCGetSlotBytes(vm, 1, &valueSize);
    size_t size = uws_req_get_header(reqRes->req, key, (size_t) valueSize, &value);
    MSCSetSlotBytes(vm, 0, value, size);
}

static void reqForEachHeaderHandler(const char *header_name, size_t header_name_size, const char *header_value,
                                    size_t header_value_size, void *user_data) {
    MVM *vm = (MVM *) user_data;
    MSCSetSlotNewMap(vm, 1);
    // push header name
    MSCSetSlotString(vm, 2, "name");
    MSCSetSlotBytes(vm, 3, header_name, header_name_size);
    MSCSetMapValue(vm, 1, 2, 3);
    // push header value
    MSCSetSlotString(vm, 2, "value");
    MSCSetSlotBytes(vm, 3, header_value, header_value_size);
    MSCSetMapValue(vm, 1, 2, 3);
    MSCInsertInList(vm, 0, -1, 1);
}

void httpServerReqHeaders(MVM *vm) {
    struct HttpRequestResponse *reqRes = (struct HttpRequestResponse *) MSCGetSlotExtern(vm, 0);
    MSCSetSlotNewList(vm, 0);
    MSCEnsureSlots(vm, 4);
    uws_req_for_each_header(reqRes->req, reqForEachHeaderHandler, vm);
}

void httpServerReqMethod(MVM *vm) {
    struct HttpRequestResponse *reqRes = (struct HttpRequestResponse *) MSCGetSlotExtern(vm, 0);
    const char *value;
    size_t size = uws_req_get_method(reqRes->req, &value);
    MSCSetSlotBytes(vm, 0, value, size);
}

void httpServerReqMethodCaseSensitive(MVM *vm) {
    struct HttpRequestResponse *reqRes = (struct HttpRequestResponse *) MSCGetSlotExtern(vm, 0);
    const char *value;
    size_t size =  uws_req_get_case_sensitive_method(reqRes->req, &value);
    MSCSetSlotBytes(vm, 0, value, size);
}

static void reqOnAbortHandler(uws_res_t *res, void *optional_data) {
    MSCHandle *cb = (MSCHandle *) optional_data;
    MVM *vm = getVM();
    MSCEnsureSlots(vm, 1);
    MSCSetSlotHandle(vm, 0, cb);
    MSCReleaseHandle(vm, cb);
    MSCCall(vm, fnCall);
}

static void reqOnDataHandler(uws_res_t *res, const char *chunk, size_t chunk_length, bool is_end, void *optional_data) {
    MSCHandle *cb = (MSCHandle *) optional_data;
    MVM *vm = getVM();
    MSCEnsureSlots(vm, 3);
    MSCSetSlotHandle(vm, 0, cb);
    MSCSetSlotBytes(vm, 1, chunk, chunk_length);
    MSCSetSlotBool(vm, 2, is_end);
    MSCCall(vm, fnCall2);
    if (is_end) {
        MSCReleaseHandle(vm, cb);
    }
}

static bool reqOnWritableHandler(uws_res_t *res, uintmax_t offset, void *optional_data) {
    MSCHandle *cb = (MSCHandle *) optional_data;
    MVM *vm = getVM();
    MSCEnsureSlots(vm, 2);
    MSCSetSlotDouble(vm, 1, offset);
    MSCSetSlotHandle(vm, 0, cb);
    MSCInterpretResult result = MSCCall(vm, fnCall1);
    if (result != RESULT_SUCCESS) {
        return false;
    }
    if (MSCGetSlotType(vm, 0) != MSC_TYPE_BOOL) {
        return false;
    }
    return MSCGetSlotBool(vm, 0);
}

void httpServerReqOnAbort(MVM *vm) {
    struct HttpRequestResponse *reqRes = (struct HttpRequestResponse *) MSCGetSlotExtern(vm, 0);
    MSCHandle *cb = MSCGetSlotHandle(vm, 1);
    reqRes->onAbort = insertInChain(reqRes->onAbort, cb);
    uws_res_on_aborted(reqRes->ssl, reqRes->res, reqOnAbortHandler, cb);
    MSCSetSlotNull(vm, 0);
}

void httpServerReqOnData(MVM *vm) {
    struct HttpRequestResponse *reqRes = (struct HttpRequestResponse *) MSCGetSlotExtern(vm, 0);
    MSCHandle *cb = MSCGetSlotHandle(vm, 1);
    uws_res_on_data(reqRes->ssl, reqRes->res, reqOnDataHandler, cb);
    MSCSetSlotNull(vm, 0);
}
void httpServerReqSetYield(MVM *vm) {
    struct HttpRequestResponse *reqRes = (struct HttpRequestResponse *) MSCGetSlotExtern(vm, 0);
    uws_req_set_yield(reqRes->req, MSCGetSlotBool(vm, 1));
    MSCSetSlotNull(vm, 0);
}
void httpServerReqYield(MVM *vm) {
    struct HttpRequestResponse *reqRes = (struct HttpRequestResponse *) MSCGetSlotExtern(vm, 0);
    MSCSetSlotBool(vm, 0, uws_req_get_yield(reqRes->req));
}
void httpServerReqIsAncient(MVM *vm) {
    struct HttpRequestResponse *reqRes = (struct HttpRequestResponse *) MSCGetSlotExtern(vm, 0);
    MSCSetSlotBool(vm, 0, uws_req_is_ancient(reqRes->req));
}

void httpServerResOnWritable(MVM *vm) {
    struct HttpRequestResponse *reqRes = (struct HttpRequestResponse *) MSCGetSlotExtern(vm, 0);
    MSCHandle *cb = MSCGetSlotHandle(vm, 1);
    reqRes->onWritable = insertInChain(reqRes->onWritable, cb);
    uws_res_on_writable(reqRes->ssl, reqRes->res, reqOnWritableHandler, cb);
    MSCSetSlotNull(vm, 0);
}

void httpServerResEnd(MVM *vm) {
    struct HttpRequestResponse *reqRes = (struct HttpRequestResponse *) MSCGetSlotExtern(vm, 0);
    int size;
    const char *data = MSCGetSlotBytes(vm, 1, &size);
    bool closeConnection = MSCGetSlotBool(vm, 2);
    uws_res_end(reqRes->ssl, reqRes->res, data, (size_t) size, closeConnection);

    MSCSetSlotNull(vm, 0);
}

void httpServerResEndWithoutBody(MVM *vm) {
    struct HttpRequestResponse *reqRes = (struct HttpRequestResponse *) MSCGetSlotExtern(vm, 0);
    bool closeConnection = MSCGetSlotBool(vm, 1);
    uws_res_end_without_body(reqRes->ssl, reqRes->res, closeConnection);
    MSCSetSlotNull(vm, 0);
}

void httpServerResPause(MVM *vm) {
    struct HttpRequestResponse *reqRes = (struct HttpRequestResponse *) MSCGetSlotExtern(vm, 0);
    uws_res_pause(reqRes->ssl, reqRes->res);
    MSCSetSlotNull(vm, 0);
}

void httpServerResResume(MVM *vm) {
    struct HttpRequestResponse *reqRes = (struct HttpRequestResponse *) MSCGetSlotExtern(vm, 0);
    uws_res_resume(reqRes->ssl, reqRes->res);
    MSCSetSlotNull(vm, 0);
}

void httpServerResWrite(MVM *vm) {
    struct HttpRequestResponse *reqRes = (struct HttpRequestResponse *) MSCGetSlotExtern(vm, 0);
    int size;
    const char *data = MSCGetSlotBytes(vm, 1, &size);
    uws_res_write(reqRes->ssl, reqRes->res, data, (size_t) size);
    MSCSetSlotNull(vm, 0);
}

void httpServerResWriteContinue(MVM *vm) {
    struct HttpRequestResponse *reqRes = (struct HttpRequestResponse *) MSCGetSlotExtern(vm, 0);
    uws_res_write_continue(reqRes->ssl, reqRes->res);
    MSCSetSlotNull(vm, 0);
}

void httpServerResWriteStatus(MVM *vm) {
    struct HttpRequestResponse *reqRes = (struct HttpRequestResponse *) MSCGetSlotExtern(vm, 0);
    int size;

    const char *status = MSCGetSlotBytes(vm, 1, &size);
    uws_res_write_status(reqRes->ssl, reqRes->res, status, (size_t) size);
    MSCSetSlotNull(vm, 0);
}

void httpServerResWriteHeader(MVM *vm) {
    struct HttpRequestResponse *reqRes = (struct HttpRequestResponse *) MSCGetSlotExtern(vm, 0);
    int keySize;
    const char *key = MSCGetSlotBytes(vm, 1, &keySize);
    int valueSize;
    const char *value = MSCGetSlotBytes(vm, 2, &valueSize);
    uws_res_write_header(reqRes->ssl, reqRes->res, key, (size_t) keySize, value, (size_t) valueSize);
    MSCSetSlotNull(vm, 0);
}

void httpServerResWriteHeaderInt(MVM *vm) {
    struct HttpRequestResponse *reqRes = (struct HttpRequestResponse *) MSCGetSlotExtern(vm, 0);
    int keySize;
    const char *key = MSCGetSlotBytes(vm, 1, &keySize);
    uint64_t value = (uint64_t) MSCGetSlotDouble(vm, 2);
    uws_res_write_header_int(reqRes->ssl, reqRes->res, key, (size_t) keySize, value);
    MSCSetSlotNull(vm, 0);
}


void httpServerResHasResponded(MVM *vm) {
    struct HttpRequestResponse *reqRes = (struct HttpRequestResponse *) MSCGetSlotExtern(vm, 0);
    MSCSetSlotBool(vm, 0, uws_res_has_responded(reqRes->ssl, reqRes->res));
}


void httpServerResTryEnd(MVM *vm) {
    struct HttpRequestResponse *reqRes = (struct HttpRequestResponse *) MSCGetSlotExtern(vm, 0);
    int size;
    const char *data = MSCGetSlotBytes(vm, 1, &size);
    uintmax_t total = (uintmax_t) MSCGetSlotDouble(vm, 2);
    bool closeConnection = MSCGetSlotBool(vm, 3);
    uws_try_end_result_t resut = uws_res_try_end(reqRes->ssl, reqRes->res, data, (size_t) size, total, closeConnection);

    MSCSetSlotBool(vm, 0, resut.ok);
}

static void uwsCorkRes(uws_res_t *res, void *userData) {
    struct HttpRequestResponse *reqRes = (struct HttpRequestResponse *) userData;
    MVM *vm = getVM();
    // update reqRes with corked res

    MSCSetSlotHandle(vm, 0, httpReqResClass);
    struct HttpRequestResponse *newReqRes = MSCSetSlotNewExtern(vm, 0, 0, sizeof(struct HttpRequestResponse));
    newReqRes->ssl = reqRes->ssl;
    newReqRes->res = res;
    newReqRes->req = reqRes->req;
    newReqRes->context = reqRes->context;
    newReqRes->onWritable = NULL;
    newReqRes->onAbort = NULL;
}

static struct HttpRequestBind *createRequest(MVM *vm) {
    struct HttpServer *server = (struct HttpServer *) MSCGetSlotExtern(vm, 0);
    struct HttpRequestBind *request = malloc(sizeof(struct HttpRequestBind));
    request->handler = MSCGetSlotHandle(vm, 2);
    request->server = server;
    request->wsHandlers = NULL;
    // add to bind list
    httpBindRequest(server, request);
    return request;
}

static struct HttpRequestBind *createWsRequest(MVM *vm) {
    struct HttpServer *server = (struct HttpServer *) MSCGetSlotExtern(vm, 0);
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

    if (MSCGetSlotType(vm, 2) == MSC_TYPE_MAP) {
        MSCSetSlotString(vm, 3, "upgrade");
        MSCGetMapValue(vm, 2, 3, 3);
        if (MSCGetSlotType(vm, 3) != MSC_TYPE_NULL) {
            request->wsHandlers->upgrade = MSCGetSlotHandle(vm, 3);
        }
        MSCSetSlotString(vm, 3, "open");
        MSCGetMapValue(vm, 2, 3, 3);
        if (MSCGetSlotType(vm, 3) != MSC_TYPE_NULL) {
            request->wsHandlers->open = MSCGetSlotHandle(vm, 3);
        }
        MSCSetSlotString(vm, 3, "close");
        MSCGetMapValue(vm, 2, 3, 3);
        if (MSCGetSlotType(vm, 3) != MSC_TYPE_NULL) {
            request->wsHandlers->close = MSCGetSlotHandle(vm, 3);
        }
        MSCSetSlotString(vm, 3, "drain");
        MSCGetMapValue(vm, 2, 3, 3);
        if (MSCGetSlotType(vm, 3) != MSC_TYPE_NULL) {
            request->wsHandlers->drain = MSCGetSlotHandle(vm, 3);
        }
        MSCSetSlotString(vm, 3, "ping");
        MSCGetMapValue(vm, 2, 3, 3);
        if (MSCGetSlotType(vm, 3) != MSC_TYPE_NULL) {
            request->wsHandlers->ping = MSCGetSlotHandle(vm, 3);
        }
        MSCSetSlotString(vm, 3, "pong");
        MSCGetMapValue(vm, 2, 3, 3);
        if (MSCGetSlotType(vm, 3) != MSC_TYPE_NULL) {
            request->wsHandlers->pong = MSCGetSlotHandle(vm, 3);
        }
        MSCSetSlotString(vm, 3, "message");
        MSCGetMapValue(vm, 2, 3, 3);
        if (MSCGetSlotType(vm, 3) != MSC_TYPE_NULL) {
            request->wsHandlers->message = MSCGetSlotHandle(vm, 3);
        }
        MSCSetSlotString(vm, 3, "subscription");
        MSCGetMapValue(vm, 2, 3, 3);
        if (MSCGetSlotType(vm, 3) != MSC_TYPE_NULL) {
            request->wsHandlers->subscription = MSCGetSlotHandle(vm, 3);
        }
    }

    // add to bind list
    httpBindRequest(server, request);
    return request;
}

void httpServerResCork(MVM *vm) {
    struct HttpRequestResponse *reqRes = (struct HttpRequestResponse *) MSCGetSlotExtern(vm, 0);
    uws_res_cork(reqRes->ssl, reqRes->res, uwsCorkRes, reqRes);
}

void ensureHttpComponentsInit(MVM *vm) {
    MSCGetVariable(vm, "net", "HttpReqRes", 0);
    httpReqResClass = MSCGetSlotHandle(vm, 0);
    MSCGetVariable(vm, "net", "HttpServer_", 0);
    httpServerClass = MSCGetSlotHandle(vm, 0);
    MSCGetVariable(vm, "net", "WebSocket", 0);
    wsSocketClass = MSCGetSlotHandle(vm, 0);
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
    MVM *vm = getVM();
    while (bindings) {
        if (bindings->value->handler != NULL) {
            MSCReleaseHandle(vm, bindings->value->handler);
        }
        if (bindings->value->wsHandlers) {
            struct WsBindings *wsBindings = bindings->value->wsHandlers;
            if (wsBindings->drain) MSCReleaseHandle(vm, wsBindings->drain);
            if (wsBindings->open) MSCReleaseHandle(vm, wsBindings->open);
            if (wsBindings->close) MSCReleaseHandle(vm, wsBindings->close);
            if (wsBindings->message) MSCReleaseHandle(vm, wsBindings->message);
            if (wsBindings->upgrade) MSCReleaseHandle(vm, wsBindings->upgrade);
            if (wsBindings->ping) MSCReleaseHandle(vm, wsBindings->ping);
            if (wsBindings->pong) MSCReleaseHandle(vm, wsBindings->pong);
            if (wsBindings->subscription) MSCReleaseHandle(vm, wsBindings->subscription);
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

void httpServerInit(MVM *vm) {
    // get options
    int ssl = (int) MSCGetSlotBool(vm, 1);

    const char *cert_file = NULL;//MSCGetSlotString(vm, 2);// cert file
    const char *key_file = NULL;//MSCGetSlotString(vm, 3);// key file
    const char *passphrase = NULL;//MSCGetSlotString(vm, 4);// passphrase
    const char *ca_file_name = NULL;//MSCGetSlotString(vm, 5);// ca_file_name
    const char *dh_params_file_name = NULL;//MSCGetSlotString(vm, 6);// dh_params_file_name
    const char *ssl_ciphers = NULL; //MSCGetSlotString(vm, 7);// ssl_ciphers
    bool ssl_prefer_low_memory_usage = false; // MSCGetSlotBool(vm, 8);// ssl_prefer_low_memory_usage
    MSCSetSlotHandle(vm, 0, httpServerClass);
    struct HttpServer *httpServer = (struct HttpServer *) MSCSetSlotNewExtern(vm, 0, 0, sizeof(struct HttpServer));
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

void httpServerGetMethod(MVM *vm) {
    const char *pattern = MSCGetSlotString(vm, 1);
    struct HttpRequestBind *request = createRequest(vm);
    uws_app_get(request->server->ssl, request->server->app, pattern, uwsHandler, request);
}


void httpServerPatchMethod(MVM *vm) {
    const char *pattern = MSCGetSlotString(vm, 1);
    struct HttpRequestBind *request = createRequest(vm);
    uws_app_patch(request->server->ssl, request->server->app, pattern, uwsHandler, request);
}

void httpServerPostMethod(MVM *vm) {
    const char *pattern = MSCGetSlotString(vm, 1);
    struct HttpRequestBind *request = createRequest(vm);
    uws_app_post(request->server->ssl, request->server->app, pattern, uwsHandler, request);
}

void httpServerPutMethod(MVM *vm) {
    const char *pattern = MSCGetSlotString(vm, 1);
    struct HttpRequestBind *request = createRequest(vm);
    uws_app_put(request->server->ssl, request->server->app, pattern, uwsHandler, request);
}

void httpServerOptionsMethod(MVM *vm) {
    const char *pattern = MSCGetSlotString(vm, 1);
    struct HttpRequestBind *request = createRequest(vm);
    uws_app_options(request->server->ssl, request->server->app, pattern, uwsHandler, request);
}

void httpServerDeleteMethod(MVM *vm) {
    const char *pattern = MSCGetSlotString(vm, 1);
    struct HttpRequestBind *request = createRequest(vm);
    uws_app_delete(request->server->ssl, request->server->app, pattern, uwsHandler, request);
}

void httpServerAnyMethod(MVM *vm) {
    const char *pattern = MSCGetSlotString(vm, 1);
    struct HttpRequestBind *request = createRequest(vm);
    uws_app_any(request->server->ssl, request->server->app, pattern, uwsHandler, request);
}
void httpServerHeadMethod(MVM *vm) {
    const char *pattern = MSCGetSlotString(vm, 1);
    struct HttpRequestBind *request = createRequest(vm);
    uws_app_head(request->server->ssl, request->server->app, pattern, uwsHandler, request);
}
void httpServerTraceMethod(MVM *vm) {
    const char *pattern = MSCGetSlotString(vm, 1);
    struct HttpRequestBind *request = createRequest(vm);
    uws_app_trace(request->server->ssl, request->server->app, pattern, uwsHandler, request);
}

void httpServerConnectMethod(MVM *vm) {
    const char *pattern = MSCGetSlotString(vm, 1);
    struct HttpRequestBind *request = createRequest(vm);
    uws_app_connect(request->server->ssl, request->server->app, pattern, uwsHandler, request);
}

static struct WebSocket *createWebSocket(MVM *vm, int slot, uws_websocket_t *ws, int ssl) {
    MSCSetSlotHandle(vm, slot, wsSocketClass);
    struct WebSocket *data = (struct WebSocket *) MSCSetSlotNewExtern(vm, slot, slot, sizeof(struct WebSocket));
    data->ssl = ssl;
    data->socket = ws;
}

static void wsOnCloseHandler(uws_websocket_t *ws, int code, const char *message, size_t length, void *user_data) {
    printf("wsOnCloseHandler:: %.*s, code: %d\n", (int)length, message, code);
    struct HttpRequestBind *handle = (struct HttpRequestBind *) user_data;
    if(!handle->wsHandlers->close) {
        return;
    }
    MVM *vm = getVM();
    MSCEnsureSlots(vm, 4);
    MSCSetSlotHandle(vm, 0, handle->wsHandlers->close);
    createWebSocket(vm, 1, ws, handle->server->ssl);
    MSCSetSlotBytes(vm, 2, message, length);
    MSCSetSlotDouble(vm, 3, code);
    MSCCall(vm, fnCall3);
}

static void wsOnOpenHandler(uws_websocket_t *ws, void *user_data) {
    printf("wsOnOpenHandler::\n");
    struct HttpRequestBind *handle = (struct HttpRequestBind *) user_data;
    if(!handle->wsHandlers->open) {
        return;
    }
    MVM *vm = getVM();
    MSCEnsureSlots(vm, 2);
    MSCSetSlotHandle(vm, 0, handle->wsHandlers->open);
    createWebSocket(vm, 1, ws, handle->server->ssl);
    MSCCall(vm, fnCall1);
}

static void wsOnDrainHandler(uws_websocket_t *ws, void *user_data) {
    printf("wsOnDrainHandler::\n");
    struct HttpRequestBind *handle = (struct HttpRequestBind *) user_data;
    if(!handle->wsHandlers->drain) {
        return;
    }
    MVM *vm = getVM();
    MSCEnsureSlots(vm, 2);
    MSCSetSlotHandle(vm, 0, handle->wsHandlers->drain);
    createWebSocket(vm, 1, ws, handle->server->ssl);
    MSCCall(vm, fnCall1);
}

static void
wsOnMessageHandler(uws_websocket_t *ws, const char *message, size_t length, uws_opcode_t opcode, void *user_data) {
    printf("wsOnMessageHandler::\n");
    struct HttpRequestBind *handle = (struct HttpRequestBind *) user_data;
    if(!handle->wsHandlers->message) {
        return;
    }
    MVM *vm = getVM();
    MSCEnsureSlots(vm, 4);
    MSCSetSlotHandle(vm, 0, handle->wsHandlers->message);
    createWebSocket(vm, 1, ws, handle->server->ssl);
    MSCSetSlotBytes(vm, 2, message, length);
    MSCSetSlotDouble(vm, 3, opcode);
    MSCCall(vm, fnCall3);
}

static void wsPongHandler(uws_websocket_t *ws, const char *message, size_t length, void *user_data) {
    printf("wsPongHandler::\n");
    struct HttpRequestBind *handle = (struct HttpRequestBind *) user_data;
    if(!handle->wsHandlers->pong) {
        return;
    }
    MVM *vm = getVM();
    MSCEnsureSlots(vm, 3);
    MSCSetSlotHandle(vm, 0, handle->wsHandlers->pong);
    createWebSocket(vm, 1, ws, handle->server->ssl);
    MSCSetSlotBytes(vm, 2, message, length);
    MSCCall(vm, fnCall2);
}

static void wsPingHandler(uws_websocket_t *ws, const char *message, size_t length, void *user_data) {
    printf("wsPingHandler::\n");
    struct HttpRequestBind *handle = (struct HttpRequestBind *) user_data;
    if(!handle->wsHandlers->ping) {
        return;
    }
    MVM *vm = getVM();
    MSCEnsureSlots(vm, 3);
    MSCSetSlotHandle(vm, 0, handle->wsHandlers->ping);
    createWebSocket(vm, 1, ws, handle->server->ssl);
    MSCSetSlotBytes(vm, 2, message, length);
    MSCCall(vm, fnCall2);
}

static void wsUpgradeHandler(uws_res_t *res, uws_req_t *req, uws_socket_context_t *context, void *user_data) {
    printf("wsUpgradeHandler::\n");
    struct HttpRequestBind *handle = (struct HttpRequestBind *) user_data;
    uws_res_on_aborted(handle->server->ssl, res, uwsOnReqAbort, NULL);
    if(!handle->wsHandlers->upgrade) {
        return;
    }
    MVM *vm = getVM();
    MSCEnsureSlots(vm, 2);
    // create a new reqres instance
    MSCSetSlotHandle(vm, 0, handle->wsHandlers->upgrade);
    MSCSetSlotHandle(vm, 1, httpReqResClass);
    struct HttpRequestResponse *reqRes = MSCSetSlotNewExtern(vm, 1, 1, sizeof(struct HttpRequestResponse));
    // (struct HttpRequestResponse *) MSCGetSlotExtern(vm, 2);
    *reqRes = (struct HttpRequestResponse) {.req = req, .res = res, .ssl = handle->server->ssl, .context = context};
    uws_res_on_aborted(reqRes->ssl, reqRes->res, uwsOnReqAbort, reqRes);
    MSCCall(vm, fnCall1);
}

static void wsSubscriptionHandler(uws_websocket_t *ws, const char *topic_name, size_t topic_name_length,
                                  int new_number_of_subscriber, int old_number_of_subscriber, void *user_data) {
    printf("wsSubscriptionHandler::\n");
    struct HttpRequestBind *handle = (struct HttpRequestBind *) user_data;
    if(!handle->wsHandlers->subscription) {
        return;
    }
    MVM *vm = getVM();
    MSCEnsureSlots(vm, 5);
    MSCSetSlotHandle(vm, 0, handle->wsHandlers->subscription);
    createWebSocket(vm, 1, ws, handle->server->ssl);
    MSCSetSlotBytes(vm, 2, topic_name, topic_name_length);
    MSCSetSlotDouble(vm, 3, new_number_of_subscriber);
    MSCSetSlotDouble(vm, 4, old_number_of_subscriber);

    MSCCall(vm, fnCall4);
}

void wsEnd(MVM *vm) {
    struct WebSocket *socket = (struct WebSocket *) MSCGetSlotExtern(vm, 0);
    int size;
    const char *message = MSCGetSlotBytes(vm, 1, &size);
    uws_ws_end(socket->ssl, socket->socket, (int) MSCGetSlotDouble(vm, 2), message, (size_t) size);
}

static void wsCorkHandler(void *userData) {

}

void wsCork(MVM *vm) {
    struct WebSocket *socket = (struct WebSocket *) MSCGetSlotExtern(vm, 0);
    uws_ws_cork(socket->ssl, socket->socket, wsCorkHandler, NULL);
}

void wsClose(MVM *vm) {
    struct WebSocket *socket = (struct WebSocket *) MSCGetSlotExtern(vm, 0);
    uws_ws_close(socket->ssl, socket->socket);
}

void wsRemoteAddress(MVM *vm) {
    struct WebSocket *socket = (struct WebSocket *) MSCGetSlotExtern(vm, 0);
    const char *dest;
    size_t size = uws_ws_get_remote_address_as_text(socket->ssl, socket->socket, &dest);
    MSCSetSlotBytes(vm, 0, dest, size);

}

void wsPublish(MVM *vm) {
    struct WebSocket *socket = (struct WebSocket *) MSCGetSlotExtern(vm, 0);
    int topicSize;
    const char *topic = MSCGetSlotBytes(vm, 1, &topicSize);
    int messageSize;
    const char *message = MSCGetSlotBytes(vm, 2, &messageSize);
    bool ok = uws_ws_publish(socket->ssl, socket->socket, topic, (size_t) topicSize, message, (size_t) messageSize);
    MSCSetSlotBool(vm, 0, ok);
}

void wsPublishWithOptions(MVM *vm) {
    struct WebSocket *socket = (struct WebSocket *) MSCGetSlotExtern(vm, 0);
    int topicSize;
    const char *topic = MSCGetSlotBytes(vm, 1, &topicSize);
    int messageSize;
    const char *message = MSCGetSlotBytes(vm, 2, &messageSize);
    bool ok = uws_ws_publish_with_options(socket->ssl, socket->socket, topic, (size_t) topicSize, message,
                                          (size_t) messageSize, (uws_opcode_t) (int) MSCGetSlotDouble(vm, 3),
                                          MSCGetSlotBool(vm, 4));
    MSCSetSlotBool(vm, 0, ok);
}

void wsSend(MVM *vm) {
    struct WebSocket *socket = (struct WebSocket *) MSCGetSlotExtern(vm, 0);
    int messageSize;
    const char *message = MSCGetSlotBytes(vm, 1, &messageSize);
    uws_sendstatus_t ok = uws_ws_send(socket->ssl, socket->socket, message, (size_t) messageSize,
                                      (uws_opcode_t) (int) MSCGetSlotDouble(vm, 2));
    MSCSetSlotDouble(vm, 0, ok);
}

void wsSubscribe(MVM *vm) {
    struct WebSocket *socket = (struct WebSocket *) MSCGetSlotExtern(vm, 0);
    int topicSize;
    const char *topic = MSCGetSlotBytes(vm, 1, &topicSize);
    bool ok = uws_ws_subscribe(socket->ssl, socket->socket, topic, (size_t) topicSize);
    MSCSetSlotBool(vm, 0, ok);
}

void wsUnsubscribe(MVM *vm) {
    struct WebSocket *socket = (struct WebSocket *) MSCGetSlotExtern(vm, 0);
    int topicSize;
    const char *topic = MSCGetSlotBytes(vm, 1, &topicSize);
    bool ok = uws_ws_unsubscribe(socket->ssl, socket->socket, topic, (size_t) topicSize);
    MSCSetSlotBool(vm, 0, ok);
}

static void wsTopicForEachHandler(const char *name, size_t size, void *userData) {
    MVM *vm = (MVM *) userData;
    // push name
    MSCSetSlotBytes(vm, 1, name, size);
    MSCSetListElement(vm, 0, -1, 2);
}

void wsTopicForEach(MVM *vm) {
    struct WebSocket *socket = (struct WebSocket *) MSCGetSlotExtern(vm, 0);
    MSCEnsureSlots(vm, 2);
    MSCSetSlotNewList(vm, 0);
    uws_ws_iterate_topics(socket->ssl, socket->socket, wsTopicForEachHandler, vm);
}

void wsBufferedAmount(MVM *vm) {
    struct WebSocket *socket = (struct WebSocket *) MSCGetSlotExtern(vm, 0);
    MSCEnsureSlots(vm, 2);
    unsigned int amount = uws_ws_get_buffered_amount(socket->ssl, socket->socket);
    MSCSetSlotDouble(vm, 0, amount);
}

void httpServerWsMethod(MVM *vm) {
    const char *pattern = MSCGetSlotString(vm, 1);
    MSCEnsureSlots(vm, 4);
    struct HttpRequestBind *request = createWsRequest(vm);

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

    if (MSCGetSlotType(vm, 2) == MSC_TYPE_MAP) {
        MSCSetSlotString(vm, 3, "idleTimeout");
        MSCGetMapValue(vm, 2, 3, 3);
        if (MSCGetSlotType(vm, 3) == MSC_TYPE_NUM) {
            behavior.idleTimeout = (unsigned short) MSCGetSlotDouble(vm, 3);
        }
        MSCSetSlotString(vm, 3, "maxBackpressure");
        MSCGetMapValue(vm, 2, 3, 3);
        if (MSCGetSlotType(vm, 3) == MSC_TYPE_NUM) {
            behavior.maxBackpressure = (unsigned int) MSCGetSlotDouble(vm, 3);
        }
        MSCSetSlotString(vm, 3, "maxLifetime");
        MSCGetMapValue(vm, 2, 3, 3);
        if (MSCGetSlotType(vm, 3) == MSC_TYPE_NUM) {
            behavior.maxLifetime = (unsigned short) MSCGetSlotDouble(vm, 3);
        }
        MSCSetSlotString(vm, 3, "maxPayloadLength");
        MSCGetMapValue(vm, 2, 3, 3);
        if (MSCGetSlotType(vm, 3) == MSC_TYPE_NUM) {
            behavior.maxPayloadLength = (unsigned int) MSCGetSlotDouble(vm, 3);
        }
        MSCSetSlotString(vm, 3, "resetIdleTimeoutOnSend");
        MSCGetMapValue(vm, 2, 3, 3);
        if (MSCGetSlotType(vm, 3) == MSC_TYPE_BOOL) {
            behavior.resetIdleTimeoutOnSend = MSCGetSlotBool(vm, 3);
        }
        MSCSetSlotString(vm, 3, "sendPingsAutomatically");
        MSCGetMapValue(vm, 2, 3, 3);
        if (MSCGetSlotType(vm, 3) == MSC_TYPE_BOOL) {
            behavior.sendPingsAutomatically = MSCGetSlotBool(vm, 3);
        }
        MSCSetSlotString(vm, 3, "closeOnBackpressureLimit");
        MSCGetMapValue(vm, 2, 3, 3);
        if (MSCGetSlotType(vm, 3) == MSC_TYPE_BOOL) {
            behavior.closeOnBackpressureLimit = MSCGetSlotBool(vm, 3);
        }
        MSCSetSlotString(vm, 3, "compression");
        MSCGetMapValue(vm, 2, 3, 3);
        if (MSCGetSlotType(vm, 3) == MSC_TYPE_BOOL) {
            behavior.compression = (uws_compress_options_t) (int) MSCGetSlotDouble(vm, 3);
        }
    }

    uws_ws(request->server->ssl, request->server->app, pattern, behavior, request);
}

static void
uwsListenHandler(struct us_listen_socket_t *listen_socket, uws_app_listen_config_t config, void *user_data) {
    MVM *vm = getVM();
    if (listen_socket) {
        MSCSetSlotBool(vm, 0, true);
    } else {
        MSCSetSlotString(vm, 0, "Failed to listen");
        MSCAbortDjuru(vm, 0);
    }

}

void httpServerListen(MVM *vm) {
    int port = (int) MSCGetSlotDouble(vm, 1);
    struct HttpServer *server = (struct HttpServer *) MSCGetSlotExtern(vm, 0);
    uws_app_listen(server->ssl, server->app, port, uwsListenHandler, NULL);
}

void httpServerStop(MVM *vm) {
    struct HttpServer *server = (struct HttpServer *) MSCGetSlotExtern(vm, 0);
    releaseApp(server);
    uws_app_close(server->ssl, server->app);
}

void httpServerPublish(MVM* vm) {
    struct HttpServer *server = (struct HttpServer *) MSCGetSlotExtern(vm, 0);
    int topicLength;
    int messageLength;
    const char* topic = MSCGetSlotBytes(vm, 1, &topicLength);
    const char* message = MSCGetSlotBytes(vm, 2, &messageLength);
    MSCSetSlotBool(vm, 0, uws_publish(server->ssl, server->app, topic, (size_t) topicLength, message, (size_t) messageLength, (uws_opcode_t)(int)MSCGetSlotDouble(vm, 3), MSCGetSlotBool(vm, 4)));
}

void httpServerNumSubscriber(MVM* vm) {
    struct HttpServer *server = (struct HttpServer *) MSCGetSlotExtern(vm, 0);
    int topicLength;
    const char* topic = MSCGetSlotBytes(vm, 1, &topicLength);
    unsigned int res = uws_num_subscribers(server->ssl, server->app, topic, (size_t) topicLength);
    MSCSetSlotDouble(vm, 0, res);
}
void httpServerRun(MVM *vm) {
    // uws_app_run(SSL, httpApp);
    // us_loop_run(uws_get_loop_with_native(getLoop()));
}

void httpShutdown() {

    MVM *vm = getVM();
    if (httpReqResClass) {
        MSCReleaseHandle(vm, httpReqResClass);
    }
    if (httpServerClass) {
        MSCReleaseHandle(vm, httpServerClass);
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
    if (fnCall3) {
        MSCReleaseHandle(vm, fnCall3);
    }
    if (fnCall4) {
        MSCReleaseHandle(vm, fnCall4);
    }
    if (wsSocketClass) {
        MSCReleaseHandle(vm, wsSocketClass);
    }
}