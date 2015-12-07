#include "base.h"
#include "log.h"

int wmain(int argc, WCHAR* argv[])
{
    DWORD Err;

    Err = GlobalLogInit(L"C:\\FbackupServer.log");
    if (Err)
        goto terminate;

    LInf("Hello world!");
    LInf("Hello world again!");

    Err = 0;

    GlobalLogRelease();
terminate:
    TerminateProcess(GetCurrentProcess(), Err);
    return 0;
}

