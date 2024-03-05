//
// Created by Mahamadou DOUMBIA [OML DSI] on 11/02/2022.
//

#include <stdlib.h>
#include <string.h>

#include "packages.h"
// #include "native_library.h"

#include "io.msc.inc"
#include "os.msc.inc"
#include "repl.msc.inc"
#include "scheduler.msc.inc"
#include "runtime.msc.inc"
#include "timer.msc.inc"
#include "cli_source.inc"
#include "nafamaw.h"

#include "ensure.msc.inc"
#include "net.msc.inc"

extern void setRootDirectory(Djuru *djuru);

extern void directoryList(Djuru *djuru);

extern void directoryCreate(Djuru *djuru);

extern void directoryDelete(Djuru *djuru);

extern void fileAllocate(Djuru *djuru);

extern void fileFinalize(void *data);

extern void fileDelete(Djuru *djuru);

extern void fileOpen(Djuru *djuru);

extern void fileSizePath(Djuru *djuru);

extern void fileClose(Djuru *djuru);

extern void fileDescriptor(Djuru *djuru);

extern void fileReadBytes(Djuru *djuru);

extern void fileRealPath(Djuru *djuru);

extern void fileSize(Djuru *djuru);

extern void fileStat(Djuru *djuru);

extern void fileWriteBytes(Djuru *djuru);

extern void platformHomePath(Djuru *djuru);

extern void platformIsPosix(Djuru *djuru);

extern void platformName(Djuru *djuru);

extern void processAllArguments(Djuru *djuru);

extern void processCwd(Djuru *djuru);

extern void processPid(Djuru *djuru);

extern void processPpid(Djuru *djuru);

extern void processVersion(Djuru *djuru);

extern void processExit(Djuru *djuru);

extern void processExec(Djuru *djuru);

extern void processChdir(Djuru *djuru);

extern void statPath(Djuru *djuru);

extern void statBlockCount(Djuru *djuru);

extern void statBlockSize(Djuru *djuru);

extern void statDevice(Djuru *djuru);

extern void statGroup(Djuru *djuru);

extern void statInode(Djuru *djuru);

extern void statLinkCount(Djuru *djuru);

extern void statMode(Djuru *djuru);

extern void statSize(Djuru *djuru);

extern void statSpecialDevice(Djuru *djuru);

extern void statUser(Djuru *djuru);

extern void statIsDirectory(Djuru *djuru);

extern void statIsFile(Djuru *djuru);

extern void stdinIsRaw(Djuru *djuru);

extern void stdinIsRawSet(Djuru *djuru);

extern void stdinIsTerminal(Djuru *djuru);

extern void stdinReadStart(Djuru *djuru);

extern void stdinReadStop(Djuru *djuru);

extern void stdoutFlush(Djuru *djuru);

extern void stderrWrite(Djuru *djuru);

extern void schedulerCaptureMethods(Djuru *djuru);
extern void scheduleNextTick(Djuru *djuru);

extern void timerStartTimer(Djuru *djuru);

extern void ensureHttpComponentsInit(Djuru *vm);

extern void httpServerInit(Djuru *djuru);

extern void httpServerDestroy(void *data);

extern void httpServerAnyMethod(Djuru *djuru);

extern void httpServerGetMethod(Djuru *djuru);

extern void httpServerDeleteMethod(Djuru *djuru);

extern void httpServerOptionsMethod(Djuru *djuru);

extern void httpServerPatchMethod(Djuru *djuru);

extern void httpServerPostMethod(Djuru *djuru);

extern void httpServerPutMethod(Djuru *djuru);

extern void httpServerHeadMethod(Djuru *djuru);

extern void httpServerConnectMethod(Djuru *djuru);

extern void httpServerTraceMethod(Djuru *djuru);

extern void httpServerWsMethod(Djuru *djuru);
// Ws

extern void wsEnd(Djuru *djuru);

extern void wsCork(Djuru *djuru);

extern void wsClose(Djuru *djuru);

extern void wsRemoteAddress(Djuru *djuru);

extern void wsPublish(Djuru *djuru);

extern void wsPublishWithOptions(Djuru *djuru);

extern void wsSend(Djuru *djuru);

extern void wsSubscribe(Djuru *djuru);

extern void wsUnsubscribe(Djuru *djuru);

extern void wsTopicForEach(Djuru *djuru);

extern void wsBufferedAmount(Djuru *djuru);

// HttpRequest methods
extern void httpServerReqDestroy(void *data);

extern void httpServerReqBody(Djuru *djuru);

extern void httpServerReqParam(Djuru *djuru);

extern void httpServerReqRemoteAddress(Djuru *djuru);

extern void httpServerReqQuery(Djuru *djuru);

extern void httpServerReqMethod(Djuru *djuru);

extern void httpServerReqMethodCaseSensitive(Djuru *djuru);

extern void httpServerReqOnAbort(Djuru *djuru);

extern void httpServerReqOnData(Djuru *djuru);

extern void httpServerReqHeader(Djuru *djuru);

extern void httpServerReqHeaders(Djuru *djuru);

extern void httpServerReqUrl(Djuru *djuru);

extern void httpServerReqFullUrl(Djuru *djuru);

extern void httpServerReqSetYield(Djuru *djuru);

extern void httpServerReqYield(Djuru *djuru);

extern void httpServerReqIsAncient(Djuru *djuru);

// HttpResponse Methods
extern void httpServerResEnd(Djuru *djuru);

extern void httpServerResEndWithoutBody(Djuru *djuru);

extern void httpServerResPause(Djuru *djuru);

extern void httpServerResResume(Djuru *djuru);

extern void httpServerResWrite(Djuru *djuru);

extern void httpServerResWriteContinue(Djuru *djuru);

extern void httpServerResWriteStatus(Djuru *djuru);

extern void httpServerResWriteHeader(Djuru *djuru);

extern void httpServerResWriteHeaderInt(Djuru *djuru);

extern void httpServerResHasResponded(Djuru *djuru);

extern void httpServerResOnWritable(Djuru *djuru);

extern void httpServerResTryEnd(Djuru *djuru);

extern void httpServerResCork(Djuru *djuru);

extern void httpServerListen(Djuru *djuru);

extern void httpServerRun(Djuru *djuru);

extern void httpServerStop(Djuru *djuru);

extern void httpServerPublish(Djuru *djuru);

extern void httpServerNumSubscriber(Djuru *djuru);

extern void socketInit(Djuru *djuru);

extern void socketDestroy(void *handle);

extern void socketBind(Djuru *djuru);

extern void socketListen(Djuru *djuru);

extern void socketAccept(Djuru *djuru);

extern void socketRead(Djuru *djuru);

extern void socketConnect(Djuru *djuru);

extern void socketWrite(Djuru *djuru);

extern void socketClose(Djuru *djuru);

extern void dnsQuery(Djuru *djuru);
extern void networkInterfaces(Djuru *djuru);



// To locate foreign classes and modules, we build a big directory for them in
// static data. The nested collection initializer syntax gets pretty noisy, so
// define a couple of macros to make it easier.
#define SENTINEL_METHOD { false, NULL, NULL }
#define SENTINEL_CLASS { NULL, { SENTINEL_METHOD } }
#define SENTINEL_MODULE {NULL, NULL, { SENTINEL_CLASS } }

#define NAMED_MODULE(name, identifier) { #name, &identifier##ModuleSource, {
#define MODULE(name) { #name, &name##ModuleSource, {
#define END_MODULE SENTINEL_CLASS } },

#define CLASS(name) { #name, {
#define END_CLASS SENTINEL_METHOD } },

#define METHOD(signature, fn) { false, signature, fn },
#define STATIC_METHOD(signature, fn) { true, signature, fn },
#define ALLOCATE(fn) { true, "<allocate>", (MSCExternMethodFn)fn },
#define FINALIZE(fn) { true, "<finalize>", (MSCExternMethodFn)fn },

// The array of built-in modules.
static ModuleRegistry coreCliModules[] =
        {
                MODULE(runtime)
                END_MODULE
                MODULE(cli)
                                CLASS(CLI)
                                                STATIC_METHOD("setRootDirectory_(_)", setRootDirectory)
                                END_CLASS
                END_MODULE
                MODULE(io)
                                CLASS(Npalan)
                                                STATIC_METHOD("create_(_,_)", directoryCreate)
                                                STATIC_METHOD("delete_(_,_)", directoryDelete)
                                                STATIC_METHOD("list_(_,_)", directoryList)
                                END_CLASS
                                CLASS(Gafe)
                                                ALLOCATE(fileAllocate)
                                                FINALIZE(fileFinalize)
                                                STATIC_METHOD("delete_(_,_)", fileDelete)
                                                STATIC_METHOD("open_(_,_,_)", fileOpen)
                                                STATIC_METHOD("realPath_(_,_)", fileRealPath)
                                                STATIC_METHOD("sizePath_(_,_)", fileSizePath)
                                                METHOD("close_(_)", fileClose)
                                                METHOD("descriptor", fileDescriptor)
                                                METHOD("readBytes_(_,_,_)", fileReadBytes)
                                                METHOD("size_(_)", fileSize)
                                                METHOD("stat_(_)", fileStat)
                                                METHOD("writeBytes_(_,_,_)", fileWriteBytes)
                                END_CLASS
                                CLASS(Stat)
                                                STATIC_METHOD("path_(_,_)", statPath)
                                                METHOD("blockCount", statBlockCount)
                                                METHOD("blockSize", statBlockSize)
                                                METHOD("device", statDevice)
                                                METHOD("group", statGroup)
                                                METHOD("inode", statInode)
                                                METHOD("linkCount", statLinkCount)
                                                METHOD("mode", statMode)
                                                METHOD("size", statSize)
                                                METHOD("specialDevice", statSpecialDevice)
                                                METHOD("user", statUser)
                                                METHOD("yeNpalan", statIsDirectory)
                                                METHOD("yeGafe", statIsFile)
                                END_CLASS
                                CLASS(Stdin)
                                                STATIC_METHOD("isRaw", stdinIsRaw)
                                                STATIC_METHOD("isRaw=(_)", stdinIsRawSet)
                                                STATIC_METHOD("isTerminal", stdinIsTerminal)
                                                STATIC_METHOD("readStart_()", stdinReadStart)
                                                STATIC_METHOD("readStop_()", stdinReadStop)
                                END_CLASS
                                CLASS(Stdout)
                                                STATIC_METHOD("flush()", stdoutFlush)
                                END_CLASS
                                CLASS(Stderr)
                                                STATIC_METHOD("write(_)", stderrWrite)
                                END_CLASS
                END_MODULE
                MODULE(os)
                                CLASS(Platform)
                                                STATIC_METHOD("homePath", platformHomePath)
                                                STATIC_METHOD("isPosix", platformIsPosix)
                                                STATIC_METHOD("name", platformName)
                                END_CLASS
                                CLASS(Process)
                                                STATIC_METHOD("allArguments", processAllArguments)
                                                STATIC_METHOD("cwd", processCwd)
                                                STATIC_METHOD("pid", processPid)
                                                STATIC_METHOD("ppid", processPpid)
                                                STATIC_METHOD("version", processVersion)
                                                STATIC_METHOD("exit_(_)", processExit)
                                                STATIC_METHOD("exec_(_,_,_,_,_)", processExec)
                                                STATIC_METHOD("chdir_(_)", processChdir)
                                END_CLASS
                END_MODULE
                MODULE(repl)
                END_MODULE
                MODULE(scheduler)
                                CLASS(DogodaBaga)
                                                STATIC_METHOD("captureMethods_()", schedulerCaptureMethods)
                                                STATIC_METHOD("nextTick(_)", scheduleNextTick)
                                END_CLASS
                END_MODULE
                MODULE(timer)
                                CLASS(WaatiMassa)
                                                STATIC_METHOD("startTimer_(_,_)", timerStartTimer)
                                END_CLASS
                END_MODULE

                MODULE(ensure)
                END_MODULE

                MODULE(net)
                                CLASS(HttpReqRes)
                                                FINALIZE(httpServerReqDestroy)
                                                METHOD("body_(_)", httpServerReqBody)
                                                METHOD("end_(_,_)", httpServerResEnd)
                                                METHOD("tryEnd_(_,_,_)", httpServerResTryEnd)
                                                METHOD("endWithNoBody_(_)", httpServerResEndWithoutBody)
                                                METHOD("corked_", httpServerResCork)
                                                METHOD("pause_()", httpServerResPause)
                                                METHOD("resume_()", httpServerResResume)
                                                METHOD("write_(_)", httpServerResWrite)
                                                METHOD("writeContinue_()", httpServerResWriteContinue)
                                                METHOD("writeHeader_(_,_)", httpServerResWriteHeader)
                                                METHOD("writeHeaderInt_(_,_)", httpServerResWriteHeaderInt)
                                                METHOD("writeStatus_(_)", httpServerResWriteStatus)
                                                METHOD("parameter_(_)", httpServerReqParam)
                                                METHOD("header_(_)", httpServerReqHeader)
                                                METHOD("headerForEach_", httpServerReqHeaders)
                                                METHOD("query_(_)", httpServerReqQuery)
                                                METHOD("hasResponded_", httpServerResHasResponded)
                                                METHOD("url_", httpServerReqUrl)
                                                METHOD("fullUrl_", httpServerReqFullUrl)
                                                METHOD("isAncient_", httpServerReqIsAncient)
                                                METHOD("yield_", httpServerReqYield)
                                                METHOD("yield_=(_)", httpServerReqSetYield)
                                                METHOD("method_", httpServerReqMethod)
                                                METHOD("remoteAddress_", httpServerReqRemoteAddress)
                                                METHOD("methodCaseSensitive_", httpServerReqMethodCaseSensitive)
                                                METHOD("onAbort_(_)", httpServerReqOnAbort)
                                                METHOD("onData_(_)", httpServerReqOnData)
                                                METHOD("onWritable_(_)", httpServerResOnWritable)
                                END_CLASS

                                CLASS(HttpServer_)
                                                ALLOCATE(httpServerInit)
                                                FINALIZE(httpServerDestroy)
                                                METHOD("any_(_,_)", httpServerAnyMethod)
                                                METHOD("delete_(_,_)", httpServerDeleteMethod)
                                                METHOD("get_(_,_)", httpServerGetMethod)
                                                METHOD("options_(_,_)", httpServerOptionsMethod)
                                                METHOD("patch_(_,_)", httpServerPatchMethod)
                                                METHOD("post_(_,_)", httpServerPostMethod)
                                                METHOD("put_(_,_)", httpServerPutMethod)
                                                METHOD("head_(_,_)", httpServerHeadMethod)
                                                METHOD("trace_(_,_)", httpServerTraceMethod)
                                                METHOD("connect_(_,_)", httpServerConnectMethod)
                                                METHOD("listen_(_)", httpServerListen)
                                                METHOD("run_()", httpServerRun)
                                                METHOD("stop_()", httpServerStop)
                                                METHOD("ws_(_,_)", httpServerWsMethod)
                                                METHOD("publish_(_,_,_,_)", httpServerPublish)
                                                METHOD("subscriberCount(_)", httpServerNumSubscriber)
                                END_CLASS
                                CLASS(WebSocket)
                                                METHOD("end(_,_)", wsEnd)
                                                METHOD("close()", wsClose)
                                                METHOD("publish(_,_)", wsPublish)
                                                METHOD("publishWithOptions(_,_,_,_)", wsPublishWithOptions)
                                                METHOD("cork()", wsCork)
                                                METHOD("subscribe(_)", wsSubscribe)
                                                METHOD("unsubscribe(_)", wsUnsubscribe)
                                                METHOD("topicForEach_(_)", wsTopicForEach)
                                                METHOD("send(_,_)", wsSend)
                                                METHOD("remoteAddress", wsRemoteAddress)
                                                METHOD("bufferedAmount", wsBufferedAmount)
                                END_CLASS
                                CLASS(HttpServer)
                                                STATIC_METHOD("ensureGlobalInit_()", ensureHttpComponentsInit)
                                END_CLASS
                                CLASS(Socket)

                                                ALLOCATE(socketInit)
                                                FINALIZE(socketDestroy)
                                                METHOD("connect(_,_)", socketConnect)
                                                METHOD("bind(_,_)", socketBind)
                                                METHOD("listen(_)", socketListen)
                                                METHOD("read()", socketRead)
                                                METHOD("write(_)", socketWrite)
                                                METHOD("accept(_)", socketAccept)
                                                METHOD("close()", socketClose)
                                END_CLASS
                                CLASS(Util)
                                                STATIC_METHOD("resolveName(_,_,_)", dnsQuery)
                                                STATIC_METHOD("netInterfaces", networkInterfaces)
                                END_CLASS

                END_MODULE

                SENTINEL_MODULE


        };

static ModuleRegistry supplementaryPackages[] =
        {
                NAMED_MODULE(filen, filen)
                END_MODULE
                SENTINEL_MODULE
        };

#undef SENTINEL_METHOD
#undef SENTINEL_CLASS
#undef SENTINEL_MODULE
#undef NAMED_MODULE
#undef MODULE
#undef END_MODULE
#undef CLASS
#undef END_CLASS
#undef METHOD
#undef STATIC_METHOD
#undef FINALIZE

static PackageRegistry packages[MAX_LIBRARIES] = {
        {"core",           (ModuleRegistry (*)[MAX_MODULES_PER_LIBRARY]) &coreCliModules},
        {"suppl_packages", (ModuleRegistry (*)[MAX_MODULES_PER_LIBRARY]) &supplementaryPackages},
        {"nafamaw",        (ModuleRegistry (*)[MAX_MODULES_PER_LIBRARY]) &nafamawModules},
        {NULL, NULL}
};

void registerPackage(const char *name, ModuleRegistry *registry) {
    int j = 0;
    while (packages[j].name != NULL) {
        j += 1;
    }
    if (j > MAX_LIBRARIES) {
        fprintf(stderr, "Too many libraries, sorry.");
        return;
    }
    packages[j].name = name;
    packages[j].modules = (ModuleRegistry (*)[MAX_MODULES_PER_LIBRARY]) registry;
}

// Looks for a built-in module with [name].
//
// Returns the BuildInModule for it or NULL if not found.
static ModuleRegistry *findModule(const char *name) {
    for (int j = 0; packages[j].name != NULL; j++) {
        ModuleRegistry *modules = &(*packages[j].modules)[0];
        for (int i = 0; modules[i].name != NULL; i++) {
            if (strcmp(name, modules[i].name) == 0) {
                return &modules[i];
            }
        }
    }

    return NULL;
}

// Looks for a class with [name] in [module].
static ClassRegistry *findClass(ModuleRegistry *module, const char *name) {
    for (int i = 0; module->classes[i].name != NULL; i++) {
        if (strcmp(name, module->classes[i].name) == 0) return &module->classes[i];
    }

    return NULL;
}

// Looks for a method with [signature] in [clas].
static MSCExternMethodFn findMethod(ClassRegistry *clas,
                                    bool isStatic, const char *signature) {
    for (int i = 0; clas->methods[i].signature != NULL; i++) {
        MethodRegistry *method = &clas->methods[i];
        if (isStatic == method->isStatic &&
            strcmp(signature, method->signature) == 0) {
            return method->method;
        }
    }

    return NULL;
}

void loadModuleComplete(MVM *vm, const char *name, struct MSCLoadModuleResult result) {
    if (result.source == NULL) return;

    free((void *) result.source);
}

MSCLoadModuleResult loadBuiltInModule(const char *name) {
    MSCLoadModuleResult result = {0};
    ModuleRegistry *module = findModule(name);
    if (module == NULL) return result;

    size_t length = strlen(*module->source);
    char *copy = (char *) malloc(length + 1);
    memcpy(copy, *module->source, length + 1);

    result.onComplete = loadModuleComplete;
    result.source = copy;
    return result;
}

MSCExternMethodFn bindBuiltInExternMethod(
        MVM *vm, const char *moduleName, const char *className, bool isStatic,
        const char *signature) {
    // TODO: Assert instead of return NULL?
    ModuleRegistry *module = findModule(moduleName);
    if (module == NULL) return NULL;

    ClassRegistry *clas = findClass(module, className);
    if (clas == NULL) return NULL;

    return findMethod(clas, isStatic, signature);
}

MSCExternClassMethods bindBuiltInExternClass(
        MVM *vm, const char *moduleName, const char *className) {
    MSCExternClassMethods methods = {NULL, NULL};

    ModuleRegistry *module = findModule(moduleName);
    if (module == NULL) return methods;

    ClassRegistry *clas = findClass(module, className);
    if (clas == NULL) return methods;

    methods.allocate = findMethod(clas, true, "<allocate>");
    methods.finalize = (MSCFinalizerFn) findMethod(clas, true, "<finalize>");

    return methods;
}