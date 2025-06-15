#include "PrintStackInfo.h"

std::string getStackTrace(int skip = 0) {
    const int maxFrames = 64;
    void* stack[maxFrames];
    HANDLE process = GetCurrentProcess();

    SymInitialize(process, NULL, TRUE);

    // Capture current call stack
    WORD frames = CaptureStackBackTrace(skip, maxFrames, stack, NULL);

    // Prepare symbol structure
    SYMBOL_INFO* symbol = (SYMBOL_INFO*)calloc(sizeof(SYMBOL_INFO) + 256, 1);
    symbol->MaxNameLen = 255;
    symbol->SizeOfStruct = sizeof(SYMBOL_INFO);

    std::ostringstream oss;
    for (int i = 0; i < frames; ++i) {
        DWORD64 address = (DWORD64)(stack[i]);
        if (SymFromAddr(process, address, 0, symbol)) {
            oss << symbol->Name;
            if (i != frames - 1)
                oss << " -> ";
        }
    }

    free(symbol);
    return oss.str();
}