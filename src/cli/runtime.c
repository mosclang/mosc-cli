//
// Created by Mahamadou DOUMBIA [OML DSI] on 11/02/2022.
//

#include <stdio.h>
#include <string.h>
#include <msc.h>
#include <net.h>
#include <MVM.h>
#include <helpers/list.h>

#include "io.h"
#include "packages.h"
#include "path.h"
#include "scheduler.h"
#include "stat.h"
#include "runtime.h"
#include "resolver.h"
#include "queue.h";
#include "list.h";

#define MICROTASK_QUEUE_SIZE 512
#define SHUTDOWN_LISTENERS_SIZE 128

// The single VM instance that the CLI uses.
static MVM *vm;

static MSCBindExternMethodFn bindMethodFn = NULL;
static MSCBindExternClassFn bindClassFn = NULL;
static MSCExternMethodFn afterLoadFn = NULL;
static MSCHandle *fnCall0 = NULL;

static uv_loop_t *loop;
static uv_async_t *microtaskAsync;
static uv_mutex_t *microtaskMutex;
static Queue *microtaskQueue;


static MSCList *shutdownListeners;

// TODO: This isn't currently used, but probably will be when package imports
// are supported. If not then, then delete this.
char *rootDirectory = NULL;
static Path *MOSCModulesDirectory = NULL;

// The exit code to use unless some other error overrides it.
int defaultExitCode = 0;

// Reads the contents of the file at [path] and returns it as a heap allocated
// string.
//
// Returns `NULL` if the path could not be found. Exits if it was found but
// could not be read.
static char *readFile(const char *path) {
    FILE *file = fopen(path, "rb");
    if (file == NULL) return NULL;

    // Find out how big the file is.
    fseek(file, 0L, SEEK_END);
    size_t fileSize = ftell(file);
    rewind(file);

    // Allocate a buffer for it.
    char *buffer = (char *) malloc(fileSize + 1);
    if (buffer == NULL) {
        fprintf(stderr, "Could not read file \"%s\".\n", path);
        exit(74);
    }

    // Read the entire file.
    size_t bytesRead = fread(buffer, 1, fileSize, file);
    if (bytesRead < fileSize) {
        fprintf(stderr, "Could not read file \"%s\".\n", path);
        exit(74);
    }

    // Terminate the string.
    buffer[bytesRead] = '\0';

    fclose(file);
    return buffer;
}

static bool isDirectory(Path *path) {
    uv_fs_t request;
    uv_fs_stat(loop, &request, path->chars, NULL);
    // TODO: Check request.result value?

    bool result = request.result == 0 &&
                  (request.statbuf.st_mode & S_IFDIR);

    uv_fs_req_cleanup(&request);
    return result;
}

static Path *realPath(Path *path) {
    uv_fs_t request;
    uv_fs_realpath(loop, &request, path->chars, NULL);

    Path *result = pathNew((char *) request.ptr);

    uv_fs_req_cleanup(&request);
    return result;
}

// Starting at [rootDirectory], walks up containing directories looking for a
// nearby "mosc_packages" directory. If found, stores it in
// [MOSCModulesDirectory].
//
// If [MOSCModulesDirectory] has already been found, does nothing.
static void findModulesDirectory() {
    if (MOSCModulesDirectory != NULL) return;

    Path *searchDirectory = pathNew(rootDirectory);
    Path *lastPath = realPath(searchDirectory);

    // Keep walking up directories as long as we find them.
    for (;;) {
        Path *modulesDirectory = pathNew(searchDirectory->chars);
        pathJoin(modulesDirectory, "mosc_packages");

        if (isDirectory(modulesDirectory)) {
            pathNormalize(modulesDirectory);
            MOSCModulesDirectory = modulesDirectory;
            break;
        }

        pathFree(modulesDirectory);

        // Walk up directories until we hit the root. We can tell that because
        // adding ".." yields the same real path.
        pathJoin(searchDirectory, "..");
        Path *thisPath = realPath(searchDirectory);
        if (strcmp(lastPath->chars, thisPath->chars) == 0) {
            pathFree(thisPath);
            break;
        }

        pathFree(lastPath);
        lastPath = thisPath;
    }

    pathFree(lastPath);
    pathFree(searchDirectory);
}

// Applies the CLI's import resolution policy. The rules are:
//
// * If [module] starts with "./" or "../", it is a relative import, relative
//   to [importer]. The resolved path is [name] concatenated onto the directory
//   containing [importer] and then normalized.
//
//   For example, importing "./a/./b/../c" from "./d/e/f" gives you "./d/e/a/c".
static const char *resolveModule(MVM *vm, const char *importer,
                                 const char *module) {
    // Logical import strings are used as-is and need no resolution.
    if (pathType(module) == PATH_TYPE_SIMPLE) return module;

    // Get the directory containing the importing module.
    Path *path = pathNew(importer);
    pathDirName(path);

    // Add the relative import path.
    pathJoin(path, module);

    pathNormalize(path);
    char *resolved = pathToString(path);

    pathFree(path);
    return resolved;
}

// Attempts to read the source for [module] relative to the current root
// directory.
//
// Returns it if found, or NULL if the module could not be found. Exits if the
// module was found but could not be read.
static MSCLoadModuleResult loadModule(MVM *vm, const char *module) {
    MSCLoadModuleResult result = {0};
    Path *filePath;
    if (pathType(module) == PATH_TYPE_SIMPLE) {
        // If there is no "mosc_packages" directory, then the only logical imports
        // we can handle are built-in ones. Let the VM try to handle it.
        findModulesDirectory();
        if (MOSCModulesDirectory == NULL) return loadBuiltInModule(module);

        // TODO: Should we explicitly check for the existence of the module's base
        // directory inside "mosc_packages" here?

        // Look up the module in "mosc_packages".
        filePath = pathNew(MOSCModulesDirectory->chars);
        pathJoin(filePath, module);

        // If the module is a single bare name, treat it as a module with the same
        // name inside the package. So "foo" means "foo/foo".
        if (strchr(module, '/') == NULL) pathJoin(filePath, module);
    } else {
        // The module path is already a file path.
        filePath = pathNew(module);
    }

    // Add a ".msc" file extension.
    pathAppendString(filePath, ".msc");

    result.onComplete = loadModuleComplete;
    result.source = readFile(filePath->chars);
    pathFree(filePath);

    // If we didn't find it, it may be a module built into the CLI or VM, so keep
    // going.
    if (result.source != NULL) return result;

    // Otherwise, see if it's a built-in module.
    return loadBuiltInModule(module);
}

// Binds foreign methods declared in either built in modules, or the injected
// API test modules.
static MSCExternMethodFn bindForeignMethod(MVM *vm, const char *module,
                                           const char *className, bool isStatic, const char *signature) {
    MSCExternMethodFn method = bindBuiltInExternMethod(vm, module, className,
                                                       isStatic, signature);
    if (method != NULL) return method;

    if (bindMethodFn != NULL) {
        return bindMethodFn(vm, module, className, isStatic, signature);
    }

    return NULL;
}

// Binds ext classes declared in either built in modules, or the injected
// API test modules.
static MSCExternClassMethods bindForeignClass(
        MVM *vm, const char *module, const char *className) {
    MSCExternClassMethods methods = bindBuiltInExternClass(vm, module,
                                                           className);
    if (methods.allocate != NULL) return methods;

    if (bindClassFn != NULL) {
        return bindClassFn(vm, module, className);
    }

    return methods;
}

static void writeFn(MVM *vm, const char *text) {
    printf("%s", text);
}

bool reportError(MVM *vm, MSCError type,
                 const char *module, int line, const char *message) {
    switch (type) {
        case ERROR_COMPILE:
            fprintf(stderr, "[%s line %d] %s\n", module, line, message);
            break;

        case ERROR_RUNTIME:
            fprintf(stderr, "%s\n", message);
            break;

        case ERROR_STACK_TRACE:
            fprintf(stderr, "[%s line %d] in %s\n", module, line, message);
            break;
    }
    return true;
}

static void deferCb(void *userData) {

}


void microtaskAsyncCb(uv_async_t *handle) {
    uv_mutex_lock(microtaskMutex);

    while (!queueIsEmpty(microtaskQueue)) {
        MSCHandle *task = queueTake(microtaskQueue);
        Djuru *djuru = getCurrentThread();
        MSCEnsureSlots(djuru, 1);
        MSCSetSlotHandle(djuru, 0, task);
        if (fnCall0 == NULL) {
            fnCall0 = MSCMakeCallHandle(vm, "weele()");
        }
        MSCCall(djuru, fnCall0);
        MSCReleaseHandle(getVM(), task);
    }
    // printf("Done Processing Async::: \n");
    // uv_unref((uv_handle_t *) microtaskAsync);
    uv_mutex_unlock(microtaskMutex);
}

void enqueueMicrotask(MSCHandle *callback) {
    uv_mutex_lock(microtaskMutex);
    enqueItem(microtaskQueue, callback);
    uv_mutex_unlock(microtaskMutex);
    // uv_ref((uv_handle_t *) microtaskAsync);
    // printf("Sending Async::: \n");
    uv_async_send(microtaskAsync);
}

void *copyHandle(const void *item) {
    return (MSCHandle *) item;
}

void destroyHandle(const void *_) {

}


void clearMicroTask() {
    MVM *vm = getVM();
    while (!queueIsEmpty(microtaskQueue)) {
        MSCHandle *task = queueTake(microtaskQueue);
        MSCReleaseHandle(vm, task);
    }
}

void registerForShutdown(ShutdownListener listener) {
    pushInList(shutdownListeners, listener);
}

static void initVM() {
    MSCConfig config;
    MSCInitConfig(&config);

    config.bindExternMethodFn = bindForeignMethod;
    config.bindExternClassFn = bindForeignClass;
    config.resolveModuleFn = resolveModule;
    config.loadModuleFn = loadModule;
    config.writeFn = writeFn;
    config.errorHandler = reportError;

    // Since we're running in a standalone process, be generous with memory.
    config.initialHeapSize = 1024 * 1024 * 100;
    vm = MSCNewVM(&config);

    // Initialize the event loop.
    loop = (uv_loop_t *) malloc(sizeof(uv_loop_t));
    uv_loop_init(loop);
    uws_loop_defer(uws_get_loop_with_native(loop), deferCb, NULL);
}

static void cleanMicroTask() {
    clearMicroTask();
    uv_mutex_destroy(microtaskMutex);
    uv_close((uv_handle_t *) microtaskAsync, NULL);
    free(microtaskAsync);
    free(microtaskMutex);
    free(microtaskQueue);
}

static void freeVM() {
    ioShutdown();
    schedulerShutdown();
    // httpShutdown();
    for (int i = 0; i < shutdownListeners->size; i++) {
        ShutdownListener listener = (ShutdownListener) getFromList(shutdownListeners, i);
        if (listener != NULL) {
            listener();
        }
    }
    cleanMicroTask();
    uv_loop_close(loop);

    MSCFreeVM(vm);

    free(loop);
    uv_tty_reset_mode();

    if (MOSCModulesDirectory != NULL) pathFree(MOSCModulesDirectory);
}

MSCInterpretResult runFile(const char *path) {
    char *source = readFile(path);
    if (source == NULL) {
        fprintf(stderr, "Could not find file \"%s\".\n", path);
        exit(66);
    }

    // If it looks like a relative path, make it explicitly relative so that we
    // can distinguish it from logical paths.
    // TODO: It might be nice to be able to run scripts from within a surrounding
    // "mosc_packages" directory by passing in a simple path like "foo/bar". In
    // that case, here, we could check to see whether the give path exists inside
    // "mosc_packages" or as a relative path and choose to add "./" or not based
    // on that.
    Path *module = pathNew(path);
    if (pathType(module->chars) == PATH_TYPE_SIMPLE) {
        Path *relative = pathNew(".");
        pathJoin(relative, path);

        pathFree(module);
        module = relative;
    }

    pathRemoveExtension(module);

    // Use the directory where the file is as the root to resolve imports
    // relative to.
    Path *directory = pathNew(module->chars);

    pathDirName(directory);
    rootDirectory = pathToString(directory);
    pathFree(directory);

    initVM();

    MSCInterpretResult result = MSCInterpret(vm, module->chars, source);

    if (afterLoadFn != NULL) afterLoadFn(vm->djuru);

    if (result == RESULT_SUCCESS) {
        uv_run(loop, UV_RUN_DEFAULT);
    }

    freeVM();

    free(source);
    free(rootDirectory);
    pathFree(module);

    return result;
}


MSCInterpretResult runCLI() {
    initResolverVM();

    // This cast is safe since we don't try to free the string later.
    rootDirectory = (char *) ".";
    shutdownListeners = newList(SHUTDOWN_LISTENERS_SIZE);
    initVM();
    microtaskQueue = initQueue(MICROTASK_QUEUE_SIZE, copyHandle, destroyHandle);
    microtaskMutex = malloc(sizeof(uv_mutex_t));
    microtaskAsync = malloc(sizeof(uv_async_t));
    uv_mutex_init(microtaskMutex);
    uv_async_init(loop, microtaskAsync, microtaskAsyncCb);
    uv_unref((uv_handle_t *) microtaskAsync);
    MSCInterpretResult result = MSCInterpret(vm, "<cli>", "kabo \"cli\" nani CLI");
    if (result == RESULT_SUCCESS) { result = MSCInterpret(vm, "<cli>", "CLI.start()"); }
    if (result == RESULT_SUCCESS) {
        uv_run(loop, UV_RUN_DEFAULT);
    }
    if (fnCall0 != NULL) {
        MSCReleaseHandle(vm, fnCall0);
    }

    freeVM();
    freeResolver();

    return result;
}

MSCInterpretResult runRepl() {
    // This cast is safe since we don't try to free the string later.
    rootDirectory = (char *) ".";
    initVM();

    printf("___    ___\n");
    printf("||\\\\__//||\n");
    printf("|| \\\\// ||\n");
    printf("||      ||\n");
    printf("{{      }} osc v%s\n", MSC_VERSION_STRING);
    // printf("\\\\/\"-\n");

    MSCInterpretResult result = MSCInterpret(vm, "<repl>", "nani \"repl\"\n");

    if (result == RESULT_SUCCESS) {
        // printf("Success\n");
        uv_run(loop, UV_RUN_DEFAULT);
    } else {
        // printf("Error\n");
    }

    freeVM();

    return result;
}

MVM *getVM() {
    return vm;
}

Djuru *getCurrentThread() {
    return MSCGetCurrentDjuru(vm);
}

uv_loop_t *getLoop() {
    return loop;
}

int getExitCode() {
    return defaultExitCode;
}

void setExitCode(int exitCode) {
    defaultExitCode = exitCode;
}
