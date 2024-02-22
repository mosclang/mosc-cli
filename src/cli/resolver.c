//
// Created by Mahamadou DOUMBIA [OML DSI] on 21/02/2022.
//

#include <string.h>
#include <stdio.h>
#include <MVM.h>

#include "resolver.h"
#include "cli_source.inc"
#include "uv.h"
#include "runtime.h"
#include "native_library.h"


MVM *resolver;


void fileLoadDynamicLibrary(Djuru *djuru) {
    const char *name = MSCGetSlotString(djuru, 1);
    const char *path = MSCGetSlotString(djuru, 2);
    // fprintf(stderr,"loading dylib %s at %s\n",name,path);

    uv_lib_t *lib = (uv_lib_t *) malloc(sizeof(uv_lib_t));
    // fprintf(stderr, "importing TIME OH BOY");
    int r = uv_dlopen(path, lib);
    if (r != 0) {
        fprintf(stderr, "error with lib %s dlopen of %s", name, path);
    }
    NativeProvider registryProvider;
    if (uv_dlsym(lib, "returnRegistry", (void **) &registryProvider)) {
        fprintf(stderr, "dlsym error: %s\n", uv_dlerror(lib));
    }
    ModuleRegistry *m = registryProvider();
    registerPackage(name, m);
}

void fileExistsSync(Djuru *djuru) {
    uv_fs_t req;
    int r = uv_fs_stat(NULL, &req, MSCGetSlotString(djuru, 1), NULL);
    // fprintf(stderr,"fileExists, %s  %d\n", MSCGetSlotString()(djuru,1), r);
    MSCEnsureSlots(djuru, 1);
    // non zero is error and means we don't have a file
    MSCSetSlotBool(djuru, 0, r == 0);
}

void fileRealPathSync(Djuru *djuru) {
    const char *path = MSCGetSlotString(djuru, 1);

    uv_fs_t request;
    uv_fs_realpath(getLoop(), &request, path, NULL);

    // fprintf("%s", request.ptr);
    // Path* result = pathNew((char*)request.ptr);
    MSCSetSlotString(djuru, 0, (const char *) request.ptr);

    uv_fs_req_cleanup(&request);
    // return result;
}

MSCHandle *resolveModuleFn;
MSCHandle *loadModuleFn;
MSCHandle *resolverClass;

void freeResolver() {
    MVM *vm = resolver;
    if (resolverClass != NULL) {
        MSCReleaseHandle(vm->djuru, resolverClass);
        MSCReleaseHandle(vm->djuru, loadModuleFn);
        MSCReleaseHandle(vm->djuru, resolveModuleFn);
        resolverClass = NULL;
        loadModuleFn = NULL;
        resolveModuleFn = NULL;
    }
    MSCFreeVM(resolver);
}

void saveResolverHandles(Djuru *djuru) {
    MSCEnsureSlots(djuru, 1);
    MSCGetVariable(djuru, "<gini>", "Gninibaga", 0);
    resolverClass = MSCGetSlotHandle(djuru, 0);
    resolveModuleFn = MSCMakeCallHandle(resolver, "moduleGnini(_,_,_)");
    loadModuleFn = MSCMakeCallHandle(resolver, "naniModuleYe(_,_)");
}

static MSCExternMethodFn bindResolverForeignMethod(MVM *vm, const char *module,
                                                   const char *className, bool isStatic, const char *signature) {
    if (strcmp(signature, "existsSync(_)") == 0) {
        return fileExistsSync;
    }
    if (strcmp(signature, "realPathSync(_)") == 0) {
        return fileRealPathSync;
    }
    if (strcmp(signature, "loadDynamicLibrary(_,_)") == 0) {
        return fileLoadDynamicLibrary;
    }
    return NULL;
}

static void writeFn(MVM *vm, const char *text) {
    printf("%s", text);
}

char *MOSCLoadModule(const char *module) {
    MVM *vm = resolver;
    Djuru* djuru = vm->djuru;
    MSCEnsureSlots(djuru, 3);
    MSCSetSlotHandle(djuru, 0, resolverClass);
    MSCSetSlotString(djuru, 1, module);
    MSCSetSlotString(djuru, 2, rootDirectory);
    int error = MSCCall(djuru, loadModuleFn);
    if (error == RESULT_RUNTIME_ERROR) {
        fprintf(stderr, "Unexpected error in Resolver.loadModule(). Cannot continue.\n");
        exit(70);
    }
    const char *tmp = MSCGetSlotString(djuru, 0);
    char *result = malloc(strlen(tmp) + 1);
    strcpy(result, tmp);
    return result;
}

char *MOSCResolveModule(const char *importer, const char *module) {
    MVM *vm = resolver;
    Djuru* djuru = vm->djuru;
    MSCEnsureSlots(djuru, 4);
    MSCSetSlotHandle(djuru, 0, resolverClass);
    MSCSetSlotString(djuru, 1, importer);
    MSCSetSlotString(djuru, 2, module);
    MSCSetSlotString(djuru, 3, rootDirectory);
    int error = MSCCall(djuru, resolveModuleFn);
    if (error == RESULT_RUNTIME_ERROR) {
        fprintf(stderr, "Unexpected error in Gninibaga.resolveModule(). Cannot continue.\n");
        exit(70);
    }
    const char *tmp = MSCGetSlotString(djuru, 0);
    char *result = malloc(strlen(tmp) + 1);
    strcpy(result, tmp);
    return result;
}

void initResolverVM() {
    MSCConfig config;
    MSCInitConfig(&config);

    config.bindExternMethodFn = bindResolverForeignMethod;
    config.writeFn = writeFn;
    config.errorHandler = reportError;

    resolver = MSCNewVM(&config);
    // printf("initResolverVM\n");
    // Initialize the event loop.
    MSCInterpretResult result = MSCInterpret(resolver, "<gini>", resolverModuleSource);
    saveResolverHandles(resolver->djuru);
}