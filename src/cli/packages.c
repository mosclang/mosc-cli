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

extern void setRootDirectory(MVM *vm);

extern void directoryList(MVM *vm);

extern void directoryCreate(MVM *vm);

extern void directoryDelete(MVM *vm);

extern void fileAllocate(MVM *vm);

extern void fileFinalize(void *data);

extern void fileDelete(MVM *vm);

extern void fileOpen(MVM *vm);

extern void fileSizePath(MVM *vm);

extern void fileClose(MVM *vm);

extern void fileDescriptor(MVM *vm);

extern void fileReadBytes(MVM *vm);

extern void fileRealPath(MVM *vm);

extern void fileSize(MVM *vm);

extern void fileStat(MVM *vm);

extern void fileWriteBytes(MVM *vm);

extern void platformHomePath(MVM *vm);

extern void platformIsPosix(MVM *vm);

extern void platformName(MVM *vm);

extern void processAllArguments(MVM *vm);

extern void processCwd(MVM *vm);

extern void processPid(MVM *vm);

extern void processPpid(MVM *vm);

extern void processVersion(MVM *vm);

extern void processExit(MVM *vm);

extern void processExec(MVM *vm);

extern void processChdir(MVM *vm);

extern void statPath(MVM *vm);

extern void statBlockCount(MVM *vm);

extern void statBlockSize(MVM *vm);

extern void statDevice(MVM *vm);

extern void statGroup(MVM *vm);

extern void statInode(MVM *vm);

extern void statLinkCount(MVM *vm);

extern void statMode(MVM *vm);

extern void statSize(MVM *vm);

extern void statSpecialDevice(MVM *vm);

extern void statUser(MVM *vm);

extern void statIsDirectory(MVM *vm);

extern void statIsFile(MVM *vm);

extern void stdinIsRaw(MVM *vm);

extern void stdinIsRawSet(MVM *vm);

extern void stdinIsTerminal(MVM *vm);

extern void stdinReadStart(MVM *vm);

extern void stdinReadStop(MVM *vm);

extern void stdoutFlush(MVM *vm);

extern void stderrWrite(MVM *vm);

extern void schedulerCaptureMethods(MVM *vm);

extern void timerStartTimer(MVM *vm);

extern void ensureHttpComponentsInit(MVM* vm);
extern void httpServerInit(MVM *vm);
extern void httpServerDestroy(void *data);
extern void httpServerAnyMethod(MVM *vm);
extern void httpServerGetMethod(MVM *vm);
extern void httpServerDeleteMethod(MVM *vm);
extern void httpServerOptionsMethod(MVM *vm);
extern void httpServerPatchMethod(MVM *vm);
extern void httpServerPostMethod(MVM *vm);
extern void httpServerPutMethod(MVM *vm);
// HttpRequest methods
extern void httpServerReqBody(MVM *vm);
extern void httpServerReqParam(MVM* vm);
extern void httpServerReqQuery(MVM* vm);
extern void httpServerReqMethod(MVM* vm);
extern void httpServerReqMethodCaseSensitive(MVM* vm);
extern void httpServerReqOnAbort(MVM* vm);
extern void httpServerReqOnData(MVM* vm);
extern void httpServerReqHeader(MVM* vm);
extern void httpServerReqHeaderForEach(MVM* vm);
extern void httpServerReqUrl(MVM* vm);
extern void httpServerReqFullUrl(MVM* vm);
// HttpResponse Methods
extern void httpServerResEnd(MVM *vm);
extern void httpServerResEndWithoutBody(MVM *vm);
extern void httpServerResPause(MVM *vm);
extern void httpServerResResume(MVM *vm);
extern void httpServerResWrite(MVM *vm);
extern void httpServerResWriteContinue(MVM *vm);
extern void httpServerResWriteStatus(MVM *vm);
extern void httpServerResWriteHeader(MVM *vm);
extern void httpServerResWriteHeaderInt(MVM *vm);
extern void httpServerResHasResponded(MVM *vm);
extern void httpServerResOnWritable(MVM *vm);
extern void httpServerResTryEnd(MVM *vm);
extern void httpServerResCork(MVM *vm);
extern void httpServerListen(MVM *vm);
extern void httpServerRun(MVM *vm);
extern void httpServerStop(MVM *vm);



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
                                                METHOD("body_(_)", httpServerReqBody)
                                                METHOD("end_(_,_)", httpServerResEnd)
                                                METHOD("tryEnd_(_,_,_)", httpServerResTryEnd)
                                                METHOD("endWithNoBody_(_)", httpServerResEndWithoutBody)
                                                METHOD("cork_(_)", httpServerResCork)
                                                METHOD("pause_()", httpServerResPause)
                                                METHOD("resume_()", httpServerResResume)
                                                METHOD("write_(_)", httpServerResWrite)
                                                METHOD("writeContinue_()", httpServerResWriteContinue)
                                                METHOD("writeHeader_(_,_)", httpServerResWriteHeader)
                                                METHOD("writeHeaderInt_(_,_)", httpServerResWriteHeaderInt)
                                                METHOD("writeStatus_(_)", httpServerResWriteStatus)
                                                METHOD("parameter_(_)", httpServerReqParam)
                                                METHOD("header_(_)", httpServerReqHeader)
                                                METHOD("headerForEach_(_)", httpServerReqHeaderForEach)
                                                METHOD("query_(_)", httpServerReqQuery)
                                                METHOD("hasResponded_", httpServerResHasResponded)
                                                METHOD("url_", httpServerReqUrl)
                                                METHOD("fullUrl_", httpServerReqFullUrl)
                                                METHOD("method_", httpServerReqMethod)
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
                                                METHOD("listen_(_)", httpServerListen)
                                                METHOD("run_()", httpServerRun)
                                                METHOD("stop_()", httpServerStop)
                                END_CLASS
                                CLASS(HttpServer)
                                                STATIC_METHOD("ensureGlobalInit_()", ensureHttpComponentsInit)
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
        {"nafamaw", (ModuleRegistry (*)[MAX_MODULES_PER_LIBRARY])&nafamawModules},
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
            if (strcmp(name, modules[i].name) == 0){
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