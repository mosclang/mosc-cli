//
// Created by Mahamadou DOUMBIA [OML DSI] on 11/02/2022.
//

#ifndef MOSC_RUNTIME_H
#define MOSC_RUNTIME_H

#include "uv.h"
#include "msc.h"

typedef void (*ShutdownListener)();
extern char* rootDirectory;

// Executes the Wren script at [path] in a new VM.
MSCInterpretResult runFile(const char* path);

// Runs the Wren interactive REPL.
MSCInterpretResult runRepl();

MSCInterpretResult runCLI();

Djuru* getCurrentThread();

// Gets the currently running VM.
MVM* getVM();


// Gets the event loop the VM is using.
uv_loop_t* getLoop();

void enqueueMicrotask(MSCHandle *callback);

void registerForShutdown(ShutdownListener listener);

// Get the exit code the CLI should exit with when done.
int getExitCode();

// Set the exit code the CLI should exit with when done.
void setExitCode(int exitCode);


bool reportError(MVM* vm, MSCError type,
                 const char* module, int line, const char* message);

#endif //MOSC_RUNTIME_H
