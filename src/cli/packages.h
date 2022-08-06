//
// Created by Mahamadou DOUMBIA [OML DSI] on 11/02/2022.
//

#ifndef MOSC_PACKAGES_H
#define MOSC_PACKAGES_H


// This wires up all of the extern classes and methods defined by the built-in
// modules bundled with the CLI.

#include "msc.h"
#include "native_library.h"

// Returns the source for built-in module [name].
MSCLoadModuleResult loadBuiltInModule(const char* module);
void loadModuleComplete(MVM* vm, const char* name, struct MSCLoadModuleResult result);

// Looks up a foreign method in a built-in module.
//
// Returns `NULL` if [moduleName] is not a built-in module.
MSCExternMethodFn bindBuiltInExternMethod(
        MVM* vm, const char* moduleName, const char* className, bool isStatic,
        const char* signature);

// Binds foreign classes declared in a built-in modules.
MSCExternClassMethods bindBuiltInExternClass(
        MVM* vm, const char* moduleName, const char* className);

#endif //MOSC_PACKAGES_H
