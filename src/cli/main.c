
#include "os.h"
#include "runtime.h"


int main(int argc, const char *argv[]) {

    osSetArguments(argc, argv);

    MSCInterpretResult result;
    runCLI();
    if (getExitCode() != 0) {
        return getExitCode();
    }

    // Exit with an error code if the script failed.
    if (result == RESULT_RUNTIME_ERROR) return 70; // EX_SOFTWARE.
    // TODO: 65 is impossible now and will need to be handled inside `cli.msc`
    if (result == RESULT_COMPILATION_ERROR) return 65; // EX_DATAERR.

    return getExitCode();
}