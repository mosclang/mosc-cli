//
// Created by Mahamadou DOUMBIA [OML DSI] on 12/02/2022.
//
#include <stdlib.h>

#include "uv.h"

#include "scheduler.h"
#include "runtime.h"

// Called by libuv when the timer finished closing.
static void timerCloseCallback(uv_handle_t *handle) {
    free(handle);
}

// Called by libuv when the timer has completed.
static void timerCallback(uv_timer_t *handle) {
    MSCHandle *fiber = (MSCHandle *) handle->data;

    // Tell libuv that we don't need the timer anymore.
    uv_close((uv_handle_t *) handle, timerCloseCallback);

    // Run the fiber that was sleeping.
    schedulerResume(fiber, false);
}

void timerStartTimer(Djuru *djuru) {
    int milliseconds = (int) MSCGetSlotDouble(djuru, 1);
    MSCHandle *fiber = MSCGetSlotHandle(djuru, 2);

    // Store the fiber to resume when the timer completes.
    uv_timer_t *handle = (uv_timer_t *) malloc(sizeof(uv_timer_t));
    handle->data = fiber;

    uv_timer_init(getLoop(), handle);
    uv_timer_start(handle, timerCallback, (uint64_t) milliseconds, 0);
}
