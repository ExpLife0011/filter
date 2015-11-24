#include <stdio.h>
#include "client.h"

BOOL IsCmdEqual(WCHAR *cmd1, WCHAR *cmd2)
{
    if (wcsncmp(cmd1, cmd2, wcslen(cmd2) + 1) == 0)
        return TRUE;
    else
        return FALSE;
}

int wmain(int argc, WCHAR* argv[])
{
    DWORD err = ERROR_INVALID_FUNCTION;
    WCHAR *cmd;

    if (argc < 2) {
        printf("Wrong num args, should be at least 1\n");
        err = ERROR_INVALID_PARAMETER;
        goto exit;
    }

    cmd = argv[1];
    printf("cmd=%ws\n", cmd);
    if (IsCmdEqual(cmd, L"load")) {
        if (argc != 3) {
            printf("Wrong num args, should be 2 args\n");
            err = ERROR_INVALID_PARAMETER;
            goto exit;
        }
        WCHAR *BinPath = argv[2];
        err = CDrvLoad(BinPath);
    } else if (IsCmdEqual(cmd, L"unload")) {
        err = CDrvUnload();
    } else if (IsCmdEqual(cmd, L"init")) {
        err = CDrvCtlInit();
    } else if (IsCmdEqual(cmd, L"release")) {
        err = CDrvCtlRelease();
    } else if (IsCmdEqual(cmd, L"echo")) {
        err = CDrvCtlEcho();
    } else if (IsCmdEqual(cmd, L"bugcheck")) {
        err = CDrvCtlBugCheck();
    } else if (IsCmdEqual(cmd, L"test")) {
        err = CDrvCtlTest();
    } else {
        printf("Unknown cmd %ws\n", cmd);
        err = ERROR_INVALID_FUNCTION;
    }

exit:
    printf("exitCode=%d\n", err);
    TerminateProcess(GetCurrentProcess(), err);

    return 0;
}

