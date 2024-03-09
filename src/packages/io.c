//
// Created by Mahamadou DOUMBIA [OML DSI] on 11/02/2022.
//

#include <stdlib.h>
#include <string.h>

#include "uv.h"

#include "scheduler.h"
#include "stat.h"
#include "runtime.h"
#include "msc.h"

#include <stdio.h>
#include <fcntl.h>
#include <Value.h>

typedef struct sFileRequestData {
    MSCHandle *fiber;
    uv_buf_t buffer;
} FileRequestData;

static const int stdinDescriptor = 0;

// Handle to the Stat class object.
static MSCHandle *statClass = NULL;

// Handle to the Stdin class object.
static MSCHandle *stdinClass = NULL;

// Handle to an onData_() method call. Called when libuv provides data on stdin.
static MSCHandle *stdinOnData = NULL;

// The stream used to read from stdin. Initialized on the first read.
static uv_stream_t *stdinStream = NULL;

// True if stdin has been set to raw mode.
static bool isStdinRaw = false;

// Frees all resources related to stdin.
static void shutdownStdin() {
    if (stdinStream != NULL) {
        uv_tty_reset_mode();
        uv_close((uv_handle_t *) stdinStream, NULL);
        free(stdinStream);
        stdinStream = NULL;
    }

    if (stdinClass != NULL) {
        MSCReleaseHandle(getVM(), stdinClass);
        stdinClass = NULL;
    }

    if (stdinOnData != NULL) {
        MSCReleaseHandle(getVM(), stdinOnData);
        stdinOnData = NULL;
    }
}

void ioShutdown() {
    shutdownStdin();

    if (statClass != NULL) {
        MSCReleaseHandle(getVM(), statClass);
        statClass = NULL;
    }
}

// If [request] failed with an error, sends the runtime error to the VM and
// frees the request.
//
// Returns true if an error was reported.
static bool handleRequestError(uv_fs_t *request) {
    if (request->result >= 0) return false;

    FileRequestData *data = (FileRequestData *) request->data;
    MSCHandle *fiber = (MSCHandle *) data->fiber;

    int error = (int) request->result;
    free(data);
    uv_fs_req_cleanup(request);
    free(request);

    schedulerResumeError(fiber, uv_strerror(error));
    return true;
}

// Allocates a new request that resumes [fiber] when it completes.
uv_fs_t *createRequest(MSCHandle *fiber) {
    uv_fs_t *request = (uv_fs_t *) malloc(sizeof(uv_fs_t));

    FileRequestData *data = (FileRequestData *) malloc(sizeof(FileRequestData));
    data->fiber = fiber;

    request->data = data;
    return request;
}

// Releases resources used by [request].
//
// Returns the fiber that should be resumed after [request] completes.
MSCHandle *freeRequest(uv_fs_t *request) {
    FileRequestData *data = (FileRequestData *) request->data;
    MSCHandle *fiber = data->fiber;

    free(data);
    uv_fs_req_cleanup(request);
    free(request);

    return fiber;
}

static void directoryListCallback(uv_fs_t *request) {
    if (handleRequestError(request)) return;

    uv_dirent_t entry;

    Djuru *djuru = getCurrentThread();
    MSCEnsureSlots(djuru, 3);
    MSCSetSlotNewList(djuru, 2);

    while (uv_fs_scandir_next(request, &entry) != UV_EOF) {
        MSCSetSlotString(djuru, 1, entry.name);
        MSCInsertInList(djuru, 2, -1, 1);
    }

    schedulerResume(freeRequest(request), true);
    schedulerFinishResume();
}

void directoryList(Djuru *djuru) {
    const char *path = MSCGetSlotString(djuru, 1);
    MSCHandle *fiber = MSCGetSlotHandle(djuru, 2);
    uv_fs_t *request = createRequest(fiber);

    int error = uv_fs_scandir(getLoop(), request, path, 0, directoryListCallback);
    if (error != 0) schedulerResumeError(fiber, uv_strerror(error));
}

void fileDirectoryCallback(uv_fs_t *request) {
    if (handleRequestError(request)) return;

    schedulerResume(freeRequest(request), false);
}

void directoryCreate(Djuru *djuru) {
    const char *path = MSCGetSlotString(djuru, 1);
    uv_fs_t *request = createRequest(MSCGetSlotHandle(djuru, 2));
    uv_fs_mkdir(getLoop(), request, path, 0, fileDirectoryCallback);
}

void directoryDelete(Djuru *djuru) {
    const char *path = MSCGetSlotString(djuru, 1);
    uv_fs_t *request = createRequest(MSCGetSlotHandle(djuru, 2));
    uv_fs_rmdir(getLoop(), request, path, fileDirectoryCallback);
}

void fileAllocate(Djuru *djuru) {
    // Store the file descriptor in the foreign data, so that we can get to it
    // in the finalizer.
    int *fd = (int *) MSCSetSlotNewExtern(djuru, 0, 0, sizeof(int));
    *fd = (int) MSCGetSlotDouble(djuru, 1);
}

void fileFinalize(void *data) {
    int fd = *(int *) data;

    // Already closed.
    if (fd == -1) return;

    uv_fs_t request;
    uv_fs_close(getLoop(), &request, fd, NULL);
    uv_fs_req_cleanup(&request);
}

static void fileDeleteCallback(uv_fs_t *request) {
    if (handleRequestError(request)) return;
    schedulerResume(freeRequest(request), false);
}

void fileDelete(Djuru *djuru) {
    const char *path = MSCGetSlotString(djuru, 1);
    MSCHandle *fiber = MSCGetSlotHandle(djuru, 2);
    uv_fs_t *request = createRequest(fiber);

    int error = uv_fs_unlink(getLoop(), request, path, fileDeleteCallback);
    if (error != 0) schedulerResumeError(fiber, uv_strerror(error));
}

static void fileOpenCallback(uv_fs_t *request) {
    if (handleRequestError(request)) return;

    double fd = (double) request->result;
    schedulerResume(freeRequest(request), true);
    MSCSetSlotDouble(getCurrentThread(), 2, fd);
    schedulerFinishResume();
}

// The UNIX file flags have specified names but not values. So we define our
// own values in FileFlags and remap them to the host OS's values here.
static int mapFileFlags(int flags) {
    int result = 0;

    // Note: These must be kept in sync with FileFlags in io.msc.
    if (flags & 0x01) result |= O_RDONLY;
    if (flags & 0x02) result |= O_WRONLY;
    if (flags & 0x04) result |= O_RDWR;
    if (flags & 0x08) result |= O_SYNC;
    if (flags & 0x10) result |= O_CREAT;
    if (flags & 0x20) result |= O_TRUNC;
    if (flags & 0x40) result |= O_EXCL;

    return result;
}

void fileOpen(Djuru *djuru) {
    const char *path = MSCGetSlotString(djuru, 1);
    int flags = (int) MSCGetSlotDouble(djuru, 2);
    uv_fs_t *request = createRequest(MSCGetSlotHandle(djuru, 3));

    // TODO: Allow controlling access.
    uv_fs_open(getLoop(), request, path, mapFileFlags(flags), S_IRUSR | S_IWUSR,
               fileOpenCallback);
}

// Called by libuv when the stat call for size completes.
static void fileSizeCallback(uv_fs_t *request) {
    if (handleRequestError(request)) return;

    double size = (double) request->statbuf.st_size;
    schedulerResume(freeRequest(request), true);
    MSCSetSlotDouble(getCurrentThread(), 2, size);
    schedulerFinishResume();
}

void fileSizePath(Djuru *djuru) {
    const char *path = MSCGetSlotString(djuru, 1);
    uv_fs_t *request = createRequest(MSCGetSlotHandle(djuru, 2));
    uv_fs_stat(getLoop(), request, path, fileSizeCallback);
}

static void fileCloseCallback(uv_fs_t *request) {
    if (handleRequestError(request)) return;

    schedulerResume(freeRequest(request), false);
}

void fileClose(Djuru *djuru) {
    int *foreign = (int *) MSCGetSlotExtern(djuru, 0);
    int fd = *foreign;

    // If it's already closed, we're done.
    if (fd == -1) {
        MSCSetSlotBool(djuru, 0, true);
        return;
    }

    // Mark it closed immediately.
    *foreign = -1;

    uv_fs_t *request = createRequest(MSCGetSlotHandle(djuru, 1));
    uv_fs_close(getLoop(), request, fd, fileCloseCallback);
    MSCSetSlotBool(djuru, 0, false);
}

void fileDescriptor(Djuru *djuru) {
    int *foreign = (int *) MSCGetSlotExtern(djuru, 0);
    int fd = *foreign;
    MSCSetSlotDouble(djuru, 0, fd);
}

static void fileReadBytesCallback(uv_fs_t *request) {
    if (handleRequestError(request)) return;

    FileRequestData *data = (FileRequestData *) request->data;
    uv_buf_t buffer = data->buffer;
    size_t count = request->result;

    // TODO: Having to copy the bytes here is a drag. It would be good if Mosc's
    // embedding API supported a way to *give* it bytes that were previously
    // allocated using Mosc's own allocator.
    schedulerResume(freeRequest(request), true);
    MSCSetSlotBytes(getCurrentThread(), 2, buffer.base, count);
    schedulerFinishResume();

    // TODO: Likewise, freeing this after we resume is lame.
    free(buffer.base);
}

void fileReadBytes(Djuru *djuru) {
    uv_fs_t *request = createRequest(MSCGetSlotHandle(djuru, 3));

    int fd = *(int *) MSCGetSlotExtern(djuru, 0);
    // TODO: Assert fd != -1.

    FileRequestData *data = (FileRequestData *) request->data;
    size_t length = (size_t) MSCGetSlotDouble(djuru, 1);
    size_t offset = (size_t) MSCGetSlotDouble(djuru, 2);

    data->buffer.len = length;
    data->buffer.base = (char *) malloc(length);

    uv_fs_read(getLoop(), request, fd, &data->buffer, 1, offset,
               fileReadBytesCallback);
}

static void realPathCallback(uv_fs_t *request) {
    if (handleRequestError(request)) return;

    MSCEnsureSlots(getCurrentThread(), 3);
    MSCSetSlotString(getCurrentThread(), 2, (char *) request->ptr);
    schedulerResume(freeRequest(request), true);
    schedulerFinishResume();
}

void fileRealPath(Djuru *djuru) {
    const char *path = MSCGetSlotString(djuru, 1);
    uv_fs_t *request = createRequest(MSCGetSlotHandle(djuru, 2));
    uv_fs_realpath(getLoop(), request, path, realPathCallback);
}

// Called by libuv when the stat call completes.
static void statCallback(uv_fs_t *request) {
    if (handleRequestError(request)) return;
    Djuru *djuru = getCurrentThread();
    MSCEnsureSlots(djuru, 3);

    // Get a handle to the Stat class. We'll hang on to this so we don't have to
    // look it up by name every time.
    if (statClass == NULL) {
        MSCGetVariable(djuru, "io", "Stat", 0);
        statClass = MSCGetSlotHandle(djuru, 0);
    }

    // Create a foreign Stat object to store the stat struct.
    MSCSetSlotHandle(djuru, 2, statClass);
    MSCSetSlotNewExtern(djuru, 2, 2, sizeof(uv_stat_t));

    // Copy the stat data.
    uv_stat_t *data = (uv_stat_t *) MSCGetSlotExtern(djuru, 2);
    *data = request->statbuf;

    schedulerResume(freeRequest(request), true);
    schedulerFinishResume();
}

void fileStat(Djuru *djuru) {
    int fd = *(int *) MSCGetSlotExtern(djuru, 0);
    uv_fs_t *request = createRequest(MSCGetSlotHandle(djuru, 1));
    uv_fs_fstat(getLoop(), request, fd, statCallback);

}

void fileSize(Djuru *djuru) {
    int fd = *(int *) MSCGetSlotExtern(djuru, 0);
    uv_fs_t *request = createRequest(MSCGetSlotHandle(djuru, 1));
    uv_fs_fstat(getLoop(), request, fd, fileSizeCallback);
}

static void fileWriteBytesCallback(uv_fs_t *request) {
    if (handleRequestError(request)) return;

    FileRequestData *data = (FileRequestData *) request->data;
    free(data->buffer.base);

    schedulerResume(freeRequest(request), false);
}

void fileWriteBytes(Djuru *djuru) {
    int fd = *(int *) MSCGetSlotExtern(djuru, 0);
    int length;
    const char *bytes = MSCGetSlotBytes(djuru, 1, &length);
    size_t offset = (size_t) MSCGetSlotDouble(djuru, 2);
    uv_fs_t *request = createRequest(MSCGetSlotHandle(djuru, 3));

    FileRequestData *data = (FileRequestData *) request->data;

    data->buffer.len = length;
    // TODO: Instead of copying, just create a MSCHandle for the byte string and
    // hold on to it in the request until the write is done.
    // TODO: Handle allocation failure.
    data->buffer.base = (char *) malloc(length);
    memcpy(data->buffer.base, bytes, length);

    uv_fs_write(getLoop(), request, fd, &data->buffer, 1, offset,
                fileWriteBytesCallback);
}

void statPath(Djuru *djuru) {
    const char *path = MSCGetSlotString(djuru, 1);
    uv_fs_t *request = createRequest(MSCGetSlotHandle(djuru, 2));
    uv_fs_stat(getLoop(), request, path, statCallback);
}

void statBlockCount(Djuru *djuru) {
    uv_stat_t *stat = (uv_stat_t *) MSCGetSlotExtern(djuru, 0);
    MSCSetSlotDouble(djuru, 0, (double) stat->st_blocks);
}

void statBlockSize(Djuru *djuru) {
    uv_stat_t *stat = (uv_stat_t *) MSCGetSlotExtern(djuru, 0);
    MSCSetSlotDouble(djuru, 0, (double) stat->st_blksize);
}

void statDevice(Djuru *djuru) {
    uv_stat_t *stat = (uv_stat_t *) MSCGetSlotExtern(djuru, 0);
    MSCSetSlotDouble(djuru, 0, (double) stat->st_dev);
}

void statGroup(Djuru *djuru) {
    uv_stat_t *stat = (uv_stat_t *) MSCGetSlotExtern(djuru, 0);
    MSCSetSlotDouble(djuru, 0, (double) stat->st_gid);
}

void statInode(Djuru *djuru) {
    uv_stat_t *stat = (uv_stat_t *) MSCGetSlotExtern(djuru, 0);
    MSCSetSlotDouble(djuru, 0, (double) stat->st_ino);
}

void statLinkCount(Djuru *djuru) {
    uv_stat_t *stat = (uv_stat_t *) MSCGetSlotExtern(djuru, 0);
    MSCSetSlotDouble(djuru, 0, (double) stat->st_nlink);
}

void statMode(Djuru *djuru) {
    uv_stat_t *stat = (uv_stat_t *) MSCGetSlotExtern(djuru, 0);
    MSCSetSlotDouble(djuru, 0, (double) stat->st_mode);
}

void statSize(Djuru *djuru) {
    uv_stat_t *stat = (uv_stat_t *) MSCGetSlotExtern(djuru, 0);
    MSCSetSlotDouble(djuru, 0, (double) stat->st_size);
}

void statSpecialDevice(Djuru *djuru) {
    uv_stat_t *stat = (uv_stat_t *) MSCGetSlotExtern(djuru, 0);
    MSCSetSlotDouble(djuru, 0, (double) stat->st_rdev);
}

void statUser(Djuru *djuru) {
    uv_stat_t *stat = (uv_stat_t *) MSCGetSlotExtern(djuru, 0);
    MSCSetSlotDouble(djuru, 0, (double) stat->st_uid);
}

void statIsDirectory(Djuru *djuru) {
    uv_stat_t *stat = (uv_stat_t *) MSCGetSlotExtern(djuru, 0);
    MSCSetSlotBool(djuru, 0, S_ISDIR(stat->st_mode));
}

void statIsFile(Djuru *djuru) {
    uv_stat_t *stat = (uv_stat_t *) MSCGetSlotExtern(djuru, 0);
    MSCSetSlotBool(djuru, 0, S_ISREG(stat->st_mode));
}

// Sets up the stdin stream if not already initialized.
static void initStdin() {
    if (stdinStream == NULL) {
        if (uv_guess_handle(stdinDescriptor) == UV_TTY) {
            // stdin is connected to a terminal.
            uv_tty_t *handle = (uv_tty_t *) malloc(sizeof(uv_tty_t));
            uv_tty_init(getLoop(), handle, stdinDescriptor, true);

            stdinStream = (uv_stream_t *) handle;
        } else {
            // stdin is a pipe or a file.
            uv_pipe_t *handle = (uv_pipe_t *) malloc(sizeof(uv_pipe_t));
            uv_pipe_init(getLoop(), handle, false);
            uv_pipe_open(handle, stdinDescriptor);
            stdinStream = (uv_stream_t *) handle;
        }
    }
}

void stdinIsRaw(Djuru *djuru) {
    MSCSetSlotBool(djuru, 0, isStdinRaw);
}

void stdinIsRawSet(Djuru *djuru) {
    initStdin();

    isStdinRaw = MSCGetSlotBool(djuru, 1);

    if (uv_guess_handle(stdinDescriptor) == UV_TTY) {
        uv_tty_t *handle = (uv_tty_t *) stdinStream;
        uv_tty_set_mode(handle, isStdinRaw ? UV_TTY_MODE_RAW : UV_TTY_MODE_NORMAL);
    } else {
        // Can't set raw mode when not talking to a TTY.
        // TODO: Make this a runtime error?
    }
}

void stdinIsTerminal(Djuru *djuru) {
    initStdin();
    MSCSetSlotBool(djuru, 0, uv_guess_handle(stdinDescriptor) == UV_TTY);
}

void stderrWrite(Djuru *djuru) {
    const char *s = MSCGetSlotString(djuru, 1);
    fprintf(stderr, "%s", s);
}


void stdoutFlush(Djuru *djuru) {
    fflush(stdout);
    MSCSetSlotNull(djuru, 0);
}

static void allocCallback(uv_handle_t *handle, size_t suggestedSize,
                          uv_buf_t *buf) {
    // TODO: Handle allocation failure.
    buf->base = (char *) malloc(suggestedSize);
    buf->len = suggestedSize;
}

static void stdinReadCallback(uv_stream_t *stream, ssize_t numRead,
                              const uv_buf_t *buffer) {
    Djuru *djuru = getCurrentThread();

    if (stdinClass == NULL) {
        MSCEnsureSlots(djuru, 1);
        MSCGetVariable(djuru, "io", "Stdin", 0);
        stdinClass = MSCGetSlotHandle(djuru, 0);
    }

    if (stdinOnData == NULL) {
        stdinOnData = MSCMakeCallHandle(getVM(), "onData_(_)");
    }

    // If stdin was closed, send null to let io.msc know.
    if (numRead == UV_EOF) {
        MSCEnsureSlots(djuru, 2);
        MSCSetSlotHandle(djuru, 0, stdinClass);
        MSCSetSlotNull(djuru, 1);
        MSCCall(djuru, stdinOnData);

        shutdownStdin();
        return;
    }

    // TODO: Handle other errors.

    // TODO: Having to copy the bytes here is a drag. It would be good if Mosc's
    // embedding API supported a way to *give* it bytes that were previously
    // allocated using Mosc's own allocator.
    MSCEnsureSlots(djuru, 2);
    MSCSetSlotHandle(djuru, 0, stdinClass);
    MSCSetSlotBytes(djuru, 1, buffer->base, numRead);
    MSCCall(djuru, stdinOnData);

    // TODO: Likewise, freeing this after we resume is lame.
    free(buffer->base);
}

void stdinReadStart(Djuru *djuru) {
    initStdin();
    uv_read_start(stdinStream, allocCallback, stdinReadCallback);
    // TODO: Check return.
}

void stdinReadStop(Djuru *djuru) {
    uv_read_stop(stdinStream);
}