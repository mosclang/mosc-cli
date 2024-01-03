//
// Created by Mahamadou DOUMBIA [OML DSI] on 12/02/2022.
//



#include <stdlib.h>
#include <string.h>

#include "uv.h"

#include "scheduler.h"
#include "msc.h"
#include "runtime.h"

// A handle to the "Scheduler" class object. Used to call static methods on it.
static MSCHandle* schedulerClass;

// This method resumes a fiber that is suspended waiting on an asynchronous
// operation. The first resumes it with zero arguments, and the second passes
// one.
static MSCHandle* resume1;
static MSCHandle* resume2;
static MSCHandle* resumeError;

static void resume(MSCHandle* method)
{
    MSCInterpretResult result = MSCCall(getVM(), method);

    // If a runtime error occurs in response to an async operation and nothing
    // catches the error in the fiber, then exit the CLI.
    if (result == RESULT_RUNTIME_ERROR)
    {
        uv_stop(getLoop());
        setExitCode(70); // EX_SOFTWARE.
    }
}

void schedulerCaptureMethods(MVM* vm)
{
    MSCEnsureSlots(vm, 1);
    MSCGetVariable(vm, "scheduler", "DogodaBaga", 0);
    schedulerClass = MSCGetSlotHandle(vm, 0);

    resume1 = MSCMakeCallHandle(vm, "resume_(_)");
    resume2 = MSCMakeCallHandle(vm, "resume_(_,_)");
    resumeError = MSCMakeCallHandle(vm, "resumeError_(_,_)");
}

void schedulerResumeAndKeepHandle(MSCHandle* fiber, bool hasArgument)
{
    MVM* vm = getVM();
    MSCEnsureSlots(vm, 2 + (hasArgument ? 1 : 0));
    MSCSetSlotHandle(vm, 0, schedulerClass);
    MSCSetSlotHandle(vm, 1, fiber);
    // MSCReleaseHandle(vm, fiber);

    // If we don't need to wait for an argument to be stored on the stack, resume
    // it now.
    if (!hasArgument) resume(resume1);
}

void schedulerResume(MSCHandle* fiber, bool hasArgument)
{
    MVM* vm = getVM();
    MSCEnsureSlots(vm, 2 + (hasArgument ? 1 : 0));
    MSCSetSlotHandle(vm, 0, schedulerClass);
    MSCSetSlotHandle(vm, 1, fiber);
    MSCReleaseHandle(vm, fiber);

    // If we don't need to wait for an argument to be stored on the stack, resume
    // it now.
    if (!hasArgument) resume(resume1);
}

void schedulerFinishResume()
{
    resume(resume2);
}

void schedulerResumeError(MSCHandle* fiber, const char* error)
{
    schedulerResume(fiber, true);
    MSCSetSlotString(getVM(), 2, error);
    resume(resumeError);
}

void schedulerShutdown()
{
    // If the module was never loaded, we don't have anything to release.
    if (schedulerClass == NULL) return;

    MVM* vm = getVM();
    MSCReleaseHandle(vm, schedulerClass);
    MSCReleaseHandle(vm, resume1);
    MSCReleaseHandle(vm, resume2);
    MSCReleaseHandle(vm, resumeError);
}