//
// Created by Mahamadou DOUMBIA [OML DSI] on 11/02/2022.
//

#include "os.h"
#include "uv.h"
#include "cli_helper.h"
#include "runtime.h"
#include "scheduler.h"

#if __APPLE__
#include "TargetConditionals.h"
#endif

typedef struct {
    MSCHandle* djuru;
    uv_process_options_t options;
} ProcessData;

int numArgs;
const char** args;

void osSetArguments(int argc, const char* argv[])
{
    numArgs = argc;
    args = argv;
}

void platformHomePath(Djuru* djuru)
{
    MSCEnsureSlots(djuru, 1);

    char _buffer[MSC_PATH_MAX];
    char* buffer = _buffer;
    size_t length = sizeof(_buffer);
    int result = uv_os_homedir(buffer, &length);

    if (result == UV_ENOBUFS)
    {
        buffer = (char*)malloc(length);
        result = uv_os_homedir(buffer, &length);
    }

    if (result != 0)
    {
        MSCSetSlotString(djuru, 0, "Cannot get the current user's home directory.");
        MSCAbortDjuru(djuru, 0);
        return;
    }

    MSCSetSlotString(djuru, 0, buffer);

    if (buffer != _buffer) free(buffer);
}

void platformName(Djuru* djuru)
{
    MSCEnsureSlots(djuru, 1);

#ifdef _WIN32
    MSCSetSlotString(djuru, 0, "Windows");
#elif __APPLE__
#if TARGET_IPHONE_SIMULATOR || TARGET_OS_IPHONE
    MSCSetSlotString(djuru, 0, "iOS");
#elif TARGET_OS_MAC
    MSCSetSlotString(djuru, 0, "OS X");
#else
    MSCSetSlotString(djuru, 0, "Unknown");
#endif
#elif __linux__
    MSCSetSlotString(djuru, 0, "Linux");
  #elif __unix__
    MSCSetSlotString(djuru, 0, "Unix");
  #elif defined(_POSIX_VERSION)
    MSCSetSlotString(djuru, 0, "POSIX");
  #else
    MSCSetSlotString(djuru, 0, "Unknown");
#endif
}

void platformIsPosix(Djuru* djuru)
{
    MSCEnsureSlots(djuru, 1);

#ifdef _WIN32
    MSCSetSlotBool(djuru, 0, false);
#elif __APPLE__
    MSCSetSlotBool(djuru, 0, true);
#elif __linux__
    MSCSetSlotBool(djuru, 0, true);
  #elif __unix__
    MSCSetSlotBool(djuru, 0, true);
  #elif defined(_POSIX_VERSION)
    MSCSetSlotBool(djuru, 0, true);
  #else
    MSCSetSlotString()(djuru, 0, false);
#endif
}

void processAllArguments(Djuru* djuru)
{
    MSCEnsureSlots(djuru, 2);
    MSCSetSlotNewList(djuru, 0);

    for (int i = 0; i < numArgs; i++)
    {
        MSCSetSlotString(djuru, 1, args[i]);
        MSCInsertInList(djuru, 0, -1, 1);
    }
}

void processCwd(Djuru* djuru)
{
    MSCEnsureSlots(djuru, 1);

    char buffer[MSC_PATH_MAX * 4];
    size_t length = sizeof(buffer);
    if (uv_cwd(buffer, &length) != 0)
    {
        MSCSetSlotString(djuru, 0, "Cannot get current working directory.");
        MSCAbortDjuru(djuru, 0);
        return;
    }

    MSCSetSlotString(djuru, 0, buffer);
}

// Called when the UV handle for a process is done, so we can free it
static void processOnClose(uv_handle_t* req)
{
    free((void*)req);
}

// Called when a process is finished running
static void processOnExit(uv_process_t* req, int64_t exit_status, int term_signal)
{
    ProcessData* data = (ProcessData*)req->data;
    MSCHandle* fiber = data->djuru;

    uv_close((uv_handle_t*)req, processOnClose);

    int index = 0;
    char* arg = data->options.args[index];
    while (arg != NULL)
    {
        free(arg);
        index += 1;
        arg = data->options.args[index];
    }

    index = 0;
    if (data->options.env) {
        char* env = data->options.env[index];
        while (env != NULL)
        {
            free(env);
            index += 1;
            env = data->options.env[index];
        }
    }

    free(data->options.stdio);
    free((void*)data);

    schedulerResume(fiber, true);
    MSCSetSlotDouble(getCurrentThread(), 2, (double)exit_status);
    schedulerFinishResume();
}

void processExit(Djuru* djuru) {
    int code = (int)MSCGetSlotDouble(djuru, 1);
    setExitCode(code);
    uv_stop(getLoop());
}

// chdir_(dir)
void processChdir(Djuru* djuru) {
    MSCEnsureSlots(djuru, 1);
    const char* dir = MSCGetSlotString(djuru, 1);
    if (uv_chdir(dir) != 0)
    {
        MSCSetSlotString(djuru, 0, "Cannot change directory.");
        MSCAbortDjuru(djuru, 0);
        return;
    }
}

//        1     2    3    4     5
// exec_(cmd, args, cwd, env, djuru)

void processExec(Djuru* djuru) {
    ProcessData* data = (ProcessData*)malloc(sizeof(ProcessData));
    memset(data, 0, sizeof(ProcessData));

    // todo: add env + cwd + flags args

    char* cmd = cli_strdup(MSCGetSlotString(djuru, 1));

    if (MSCGetSlotType(djuru, 3) != MSC_TYPE_NULL) {
        const char* cwd = MSCGetSlotString(djuru, 3);
        data->options.cwd = cwd;
    }

    // input/output: for now we'll hookup STDOUT/STDERR as inherit/passthru so
    // we'll see output just like you would in a shell script, by default
    data->options.stdio_count = 3;
    // TODO: make more flexible
    uv_stdio_container_t *child_stdio = malloc(sizeof(uv_stdio_container_t) * 3);
    memset(child_stdio, 0, sizeof(uv_stdio_container_t) * 3);
    child_stdio[0].flags = UV_IGNORE;
    child_stdio[1].flags = UV_INHERIT_FD;
    child_stdio[2].flags = UV_INHERIT_FD;
    child_stdio[1].data.fd = 1;
    child_stdio[2].data.fd = 2;
    data->options.stdio = child_stdio;

    data->options.file = cmd;
    data->options.exit_cb = processOnExit;
    data->djuru = MSCGetSlotHandle(djuru, 5);

    MSCEnsureSlots(djuru, 7);

    if (MSCGetSlotType(djuru, 4) == MSC_TYPE_NULL) {
        // no environment specified
    } else if (MSCGetSlotType(djuru, 4) == MSC_TYPE_LIST) {
        int envCount = MSCGetListCount(djuru, 4);
        int envSize = sizeof(char*) * (envCount + 1);

        data->options.env = (char**)malloc(envSize);
        data->options.env[envCount] = NULL;

        for (int i = 0; i < envCount ; i++)
        {
            MSCGetListElement(djuru, 4, i, 6);
            if (MSCGetSlotType(djuru, 6) != MSC_TYPE_STRING) {
                MSCSetSlotString(djuru, 0, "arguments to env are supposed to be strings");
                MSCAbortDjuru(djuru, 0);
            }
            char* envKeyPlusValue = cli_strdup(MSCGetSlotString(djuru, 6));
            data->options.env[i] = envKeyPlusValue;
        }
    }

    int argCount = MSCGetListCount(djuru, 2);
    int argsSize = sizeof(char*) * (argCount + 2);

    // First argument is the cmd, last+1 is NULL
    data->options.args = (char**)malloc(argsSize);
    data->options.args[0] = cmd;
    data->options.args[argCount + 1] = NULL;

    for (int i = 0; i < argCount; i++)
    {
        MSCGetListElement(djuru, 2, i, 3);
        if (MSCGetSlotType(djuru, 3) != MSC_TYPE_STRING) {
            MSCSetSlotString(djuru, 0, "arguments to args are supposed to be strings");
            MSCAbortDjuru(djuru, 0);
        }
        char* arg = cli_strdup(MSCGetSlotString(djuru, 3));
        data->options.args[i + 1] = arg;
    }

    uv_process_t* child_req = (uv_process_t*)malloc(sizeof(uv_process_t));
    memset(child_req, 0, sizeof(uv_process_t));

    child_req->data = data;

    int r;
    if ((r = uv_spawn(getLoop(), child_req, &data->options)))
    {
        // should be stderr??? but no idea how to make tests work/pass with that
        fprintf(stdout, "Could not launch %s, reason: %s\n", cmd, uv_strerror(r));
        MSCSetSlotString(djuru, 0, "Could not spawn process.");
        MSCReleaseHandle(djuru, data->djuru);
        MSCAbortDjuru(djuru, 0);
    }
}

void processPid(Djuru* djuru) {
    MSCEnsureSlots(djuru, 1);
    MSCSetSlotDouble(djuru, 0, uv_os_getpid());
}

void processPpid(Djuru* djuru) {
    MSCEnsureSlots(djuru, 1);
    MSCSetSlotDouble(djuru, 0, uv_os_getppid());
}

void processVersion(Djuru* djuru) {
    MSCEnsureSlots(djuru, 1);
    MSCSetSlotString(djuru, 0, MSC_VERSION_STRING);
}