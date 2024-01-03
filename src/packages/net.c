//
// Created by Mahamadou DOUMBIA [OML DSI] on 28/12/2023.
//

#include <MVM.h>
#include <debuger.h>
#include <libuwebsockets.h>
#include "net.h"
#include "runtime.h"
#include "scheduler.h"


struct ByteArrayList {
    const char *data;
    size_t length;
    struct ByteArrayList *next;
};
struct HttpRequestResponse {
    int ssl;
    uws_res_t *res;
    uws_req_t *req;
};
struct HttpOnDataParser {
    void *data;
    struct ByteArrayList *body;
    struct HttpRequestResponse *reqRes;
    MSCHandle *handler;
};
struct HttpServer {
    int ssl;
    uws_app_t *app;
    struct us_socket_context_options_t options;
    struct HttpRequestBindBuffer *bindings;
};

struct HttpRequestBind {
    struct HttpServer *server;
    MSCHandle *handler;
};

struct HttpRequestBindBuffer {
    struct HttpRequestBind *value;
    struct HttpRequestBindBuffer *next;
};

static MSCHandle *httpReqResClass;
static MSCHandle *httpServerClass;
static MSCHandle *fnCall1;


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

static void uwsOnResAbort(uws_res_t *res, void *optional_data) {
    struct HttpOnDataParser *parser = (struct HttpOnDataParser *) optional_data;
    // schedulerResumeError(parser->handler, "Request aborted");
    free(parser);

}

static void uwsOnReqAbort(uws_res_t *res, void *optional_data) {
    // struct HttpRequestResponse *reqRes = (struct HttpRequestResponse *) optional_data;
    // schedulerResumeError(handle->handler, "Request aborted");
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
    struct HttpRequestBind *handle = (struct HttpRequestBind *) user_data;
    MSCEnsureSlots(vm, 2);
    // create a new request instance
    MSCSetSlotHandle(vm, 0, handle->handler);
    MSCSetSlotHandle(vm, 1, httpReqResClass);
    struct HttpRequestResponse *reqRes = MSCSetSlotNewExtern(vm, 1, 1, sizeof(struct HttpRequestResponse));
    // (struct HttpRequestResponse *) MSCGetSlotExtern(vm, 2);
    *reqRes = (struct HttpRequestResponse) {.req = req, .res = res, .ssl = handle->server->ssl};
    uws_res_on_aborted(reqRes->ssl, reqRes->res, uwsOnReqAbort, reqRes);
    // schedulerResumeAndKeepHandle(handle->handler, true);
    // schedulerFinishResume();
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

void httpServerReqUrl(MVM *vm) {
    struct HttpRequestResponse *reqRes = (struct HttpRequestResponse *) MSCGetSlotExtern(vm, 0);
    const char *value;
    uws_req_get_url(reqRes->req, &value);
    MSCSetSlotString(vm, 0, value);
}

void httpServerReqFullUrl(MVM *vm) {
    struct HttpRequestResponse *reqRes = (struct HttpRequestResponse *) MSCGetSlotExtern(vm, 0);
    const char *value;
    uws_req_get_full_url(reqRes->req, &value);
    MSCSetSlotString(vm, 0, value);
}

void httpServerReqQuery(MVM *vm) {
    struct HttpRequestResponse *reqRes = (struct HttpRequestResponse *) MSCGetSlotExtern(vm, 0);
    const char *value;
    int size;
    const char *key = MSCGetSlotBytes(vm, 1, &size);
    uws_req_get_query(reqRes->req, key, (size_t) size, &value);
    MSCSetSlotString(vm, 0, value);
}

void httpServerReqHeader(MVM *vm) {
    struct HttpRequestResponse *reqRes = (struct HttpRequestResponse *) MSCGetSlotExtern(vm, 0);
    const char *value;
    int size;
    const char *key = MSCGetSlotBytes(vm, 1, &size);
    uws_req_get_header(reqRes->req, key, (size_t) size, &value);
    MSCSetSlotString(vm, 0, value);
}

static void forEachHeaderHandler(const char *header_name, size_t header_name_size, const char *header_value,
                                 size_t header_value_size, void *user_data) {
    MSCHandle *cb = (MSCHandle *) user_data;
    MVM *vm = getVM();
    MSCEnsureSlots(vm, 3);
    MSCSetSlotNewList(vm, 1);
    // push header name
    MSCSetSlotString(vm, 2, header_name);
    MSCInsertInList(vm, 0, -1, 2);
    // push header value
    MSCSetSlotString(vm, 2, header_value);
    MSCInsertInList(vm, 0, -1, 2);
    MSCSetSlotHandle(vm, 0, cb);
    MSCCall(vm, cb);
}

void httpServerReqHeaderForEach(MVM *vm) {
    struct HttpRequestResponse *reqRes = (struct HttpRequestResponse *) MSCGetSlotExtern(vm, 0);
    MSCHandle *cb = MSCGetSlotHandle(vm, 1);
    uws_req_for_each_header(reqRes->req, forEachHeaderHandler, cb);
}

void httpServerReqMethod(MVM *vm) {
    struct HttpRequestResponse *reqRes = (struct HttpRequestResponse *) MSCGetSlotExtern(vm, 0);
    const char *value;
    uws_req_get_method(reqRes->req, &value);
    MSCSetSlotString(vm, 0, value);
}

void httpServerReqMethodCaseSensitive(MVM *vm) {
    struct HttpRequestResponse *reqRes = (struct HttpRequestResponse *) MSCGetSlotExtern(vm, 0);
    const char *value;
    uws_req_get_case_sensitive_method(reqRes->req, &value);
    MSCSetSlotString(vm, 0, value);
}

static void reqOnAbortHandler(uws_res_t *res, void *optional_data) {
    MSCHandle *cb = (MSCHandle *) optional_data;
    MVM *vm = getVM();
    MSCEnsureSlots(vm, 1);
    MSCSetSlotHandle(vm, 0, cb);
    MSCCall(vm, cb);
}

static void reqOnDataHandler(uws_res_t *res, const char *chunk, size_t chunk_length, bool is_end, void *optional_data) {
    MSCHandle *cb = (MSCHandle *) optional_data;
    MVM *vm = getVM();
    MSCEnsureSlots(vm, 3);
    MSCSetSlotNewList(vm, 1);
    MSCSetSlotString(vm, 2, chunk);
    MSCInsertInList(vm, 1, -1, 2);
    MSCSetSlotBool(vm, 2, is_end);
    MSCInsertInList(vm, 1, -1, 2);
    MSCSetSlotHandle(vm, 0, cb);
    MSCCall(vm, cb);
}

static bool reqOnWritableHandler(uws_res_t *res, uintmax_t offset, void *optional_data) {
    MSCHandle *cb = (MSCHandle *) optional_data;
    MVM *vm = getVM();
    MSCEnsureSlots(vm, 2);
    MSCSetSlotDouble(vm, 1, offset);
    MSCSetSlotHandle(vm, 0, cb);
    MSCInterpretResult result = MSCCall(vm, cb);
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
    uws_res_on_aborted(reqRes->ssl, reqRes->res, reqOnAbortHandler, cb);
    MSCSetSlotNull(vm, 0);
}

void httpServerReqOnData(MVM *vm) {
    struct HttpRequestResponse *reqRes = (struct HttpRequestResponse *) MSCGetSlotExtern(vm, 0);
    MSCHandle *cb = MSCGetSlotHandle(vm, 1);
    uws_res_on_data(reqRes->ssl, reqRes->res, reqOnDataHandler, cb);
    MSCSetSlotNull(vm, 0);
}

void httpServerResOnWritable(MVM *vm) {
    struct HttpRequestResponse *reqRes = (struct HttpRequestResponse *) MSCGetSlotExtern(vm, 0);
    MSCHandle *cb = MSCGetSlotHandle(vm, 1);
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
    uws_res_write_header(reqRes->ssl, reqRes->res, key, (size_t) keySize, value, (size_t)valueSize);
    MSCSetSlotNull(vm, 0);
}
void httpServerResWriteHeaderInt(MVM *vm) {
    struct HttpRequestResponse *reqRes = (struct HttpRequestResponse *) MSCGetSlotExtern(vm, 0);
    int keySize;
    const char *key = MSCGetSlotBytes(vm, 1, &keySize);
    uint64_t value = (uint64_t)MSCGetSlotDouble(vm, 2);
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
    struct HttpRequestResponse newReqRes = {
            .res = res,
            .req = reqRes->req
    };
    MSCSetSlotHandle(vm, 2, httpReqResClass);
    MSCSetSlotNewExtern(vm, 2, 2, sizeof(struct HttpRequestResponse));
    *((struct HttpRequestResponse *) MSCGetSlotExtern(vm, 2)) = newReqRes;
    schedulerResume(MSCGetSlotHandle(vm, 1), true);
    schedulerFinishResume();
}

static struct HttpRequestBind *createRequest(MVM *vm) {
    struct HttpServer *server = (struct HttpServer *) MSCGetSlotExtern(vm, 0);
    struct HttpRequestBind *request = malloc(sizeof(struct HttpRequestBind));
    request->handler = MSCGetSlotHandle(vm, 2);
    request->server = server;
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
    fnCall1 = MSCMakeCallHandle(vm, "weele(_)");

}

void httpServerDestroy(void *data) {
    struct HttpServer *server = (struct HttpServer *) data;
    struct HttpRequestBindBuffer *bindings = server->bindings;
    MVM *vm = getVM();
    while (bindings) {
        MSCReleaseHandle(vm, bindings->value->handler);
        struct HttpRequestBindBuffer *old = bindings;
        bindings = bindings->next;
        free(old->value);
        free(old);
    }
    uws_app_destroy(server->ssl, server->app);
    free(server);
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
/*void httpServerWsMethod(MVM *vm) {
    const char *pattern = MSCGetSlotString(vm, 1);
    struct HttpRequestBind *request = createRequest(vm);
    uws_ws(request->server->ssl, request->server->app, pattern, uwsHandler, request);
}*/

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
    uws_app_close(server->ssl, server->app);

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
    if (fnCall1) {
        MSCReleaseHandle(vm, fnCall1);
    }
}