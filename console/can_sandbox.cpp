#include "can_sandbox_lib.h"

int main(int argc, const char* argv[]) {
    CanSandbox sandbox;

    if (sandbox.parseOptions(argc, argv)) {
        sandbox.startProcessing();
        return 0;
    }
    return 1;
}
