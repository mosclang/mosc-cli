//
// Created by Mahamadou DOUMBIA [OML DSI] on 11/02/2022.
//

#ifndef MOSC_RUNTIME_H
#define MOSC_RUNTIME_H

#include "uv.h"
#include "msc.h"

extern char* rootDirectory;

// Executes the Wren script at [path] in a new VM.
MSCInterpretResult runFile(const char* path);

// Runs the Wren interactive REPL.
MSCInterpretResult runRepl();

MSCInterpretResult runCLI();

// Gets the currently running VM.
MVM* getVM();

// Gets the event loop the VM is using.
uv_loop_t* getLoop();

// Get the exit code the CLI should exit with when done.
int getExitCode();

// Set the exit code the CLI should exit with when done.
void setExitCode(int exitCode);

// Adds additional callbacks to use when binding foreign members from Wren.
//
// Used by the API test executable to let it wire up its own foreign functions.
// This must be called before calling [createVM()].
void setTestCallbacks(MSCBindExternMethodFn bindMethod,
                      MSCBindExternClassFn bindClass,
                      void (*afterLoad)(MVM* vm));
bool reportError(MVM* vm, MSCError type,
                 const char* module, int line, const char* message);

#endif //MOSC_RUNTIME_H
