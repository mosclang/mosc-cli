//
// Created by Mahamadou DOUMBIA [OML DSI] on 12/02/2022.
//



#include <stdlib.h>
#include <string.h>
#include <Value.h>

#include "uv.h"

#include "scheduler.h"
#include "msc.h"
#include "runtime.h"

// A handle to the "Scheduler" class object. Used to call static methods on it.
static MSCHandle *schedulerClass;

// This method resumes a fiber that is suspended waiting on an asynchronous
// operation. The first resumes it with zero arguments, and the second passes
// one.
static MSCHandle *resume1;
static MSCHandle *resume2;
static MSCHandle *resumeError;

static void resume(MSCHandle *method) {
    MSCInterpretResult result = MSCCall(getCurrentThread(), method);

    // If a runtime error occurs in response to an async operation and nothing
    // catches the error in the fiber, then exit the CLI.
    if (result == RESULT_RUNTIME_ERROR) {
        uv_stop(getLoop());
        setExitCode(70); // EX_SOFTWARE.
    }
}

void schedulerCaptureMethods(Djuru *djuru) {
    MSCEnsureSlots(djuru, 1);
    MSCGetVariable(djuru, "scheduler", "DogodaBaga", 0);
    schedulerClass = MSCGetSlotHandle(djuru, 0);

    resume1 = MSCMakeCallHandle(djuru->vm, "resume_(_)");
    resume2 = MSCMakeCallHandle(djuru->vm, "resume_(_,_)");
    resumeError = MSCMakeCallHandle(djuru->vm, "resumeError_(_,_)");
}


void schedulerResume(MSCHandle *fiber, bool hasArgument) {
    Djuru *djuru = getCurrentThread();
    MSCEnsureSlots(djuru, 2 + (hasArgument ? 1 : 0));
    MSCSetSlotHandle(djuru, 0, schedulerClass);
    MSCSetSlotHandle(djuru, 1, fiber);
    MSCReleaseHandle(djuru, fiber);

    // If we don't need to wait for an argument to be stored on the stack, resume
    // it now.
    if (!hasArgument) resume(resume1);
}

void schedulerFinishResume() {
    resume(resume2);
}

void schedulerResumeError(MSCHandle *fiber, const char *error) {
    schedulerResume(fiber, true);
    MSCSetSlotString(getCurrentThread(), 2, error);
    resume(resumeError);
}

void schedulerShutdown() {
    // If the module was never loaded, we don't have anything to release.
    if (schedulerClass == NULL) return;

    Djuru *djuru = getCurrentThread();
    MSCReleaseHandle(djuru, schedulerClass);
    MSCReleaseHandle(djuru, resume1);
    MSCReleaseHandle(djuru, resume2);
    MSCReleaseHandle(djuru, resumeError);
}

void scheduleNextTick(Djuru *djuru) {
    MSCHandle *cb = MSCGetSlotHandle(djuru, 1);
    enqueueMicrotask(cb);
}