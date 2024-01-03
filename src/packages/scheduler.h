//
// Created by Mahamadou DOUMBIA [OML DSI] on 12/02/2022.
//

#ifndef MOSC_SCHEDULER_H
#define MOSC_SCHEDULER_H


#include "msc.h"

// Sets up the API stack to call one of the resume methods on Scheduler.
//
// If [hasArgument] is false, this just sets up the stack to have another
// argument stored in slot 2 and returns. The module must store the argument
// on the stack and then call [schedulerFinishResume] to complete the call.
//
// Otherwise, the call resumes immediately. Releases [fiber] when called.
void schedulerResume(MSCHandle* fiber, bool hasArgument);
void schedulerResumeAndKeepHandle(MSCHandle* fiber, bool hasArgument);

void schedulerFinishResume();
void schedulerResumeError(MSCHandle* fiber, const char* error);

void schedulerShutdown();

#endif //MOSC_SCHEDULER_H
