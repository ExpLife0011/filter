#include "client.h"
#include "srvapi.h"
#include "..\server\api.h"

DWORD SrvPrintTime(PWCHAR Host)
{
    DWORD Err;
    PSRV_API_CTX ApiCtx;
    SYSTEMTIME Time;

    Err = SrvApiInit();
    if (Err)
        return Err;

    Err = SrvApiConnect(Host, FB_SRV_PORT, &ApiCtx);
    if (Err)
        return Err;

    Err = SrvApiGetTime(ApiCtx, &Time);
    if (Err)
        goto api_close;

    printf("Server time %02d:%02d:%02d.%03d\n",
           Time.wHour, Time.wMinute,
           Time.wSecond, Time.wMilliseconds);

api_close:
    SrvApiClose(ApiCtx);
    SrvApiRelease();
    return Err;
}

BOOL IsCmdEqual(WCHAR *cmd1, WCHAR *cmd2)
{
    if (wcsncmp(cmd1, cmd2, wcslen(cmd1) + 1) == 0)
        return TRUE;
    else
        return FALSE;
}

int wmain(int argc, WCHAR* argv[])
{
    DWORD Err = ERROR_INVALID_FUNCTION;
    WCHAR *Cmd;

    if (argc < 2) {
        printf("Wrong num args, should be at least 1\n");
        Err = ERROR_INVALID_PARAMETER;
        goto exit;
    }

    Cmd = argv[1];
    printf("Cmd %ws\n", Cmd);
    if (IsCmdEqual(Cmd, L"load")) {
        if (argc != 3) {
            printf("Wrong num args, should be 2 args\n");
            Err = ERROR_INVALID_PARAMETER;
            goto exit;
        }
        WCHAR *BinPath = argv[2];
        Err = CDrvLoad(BinPath);
    } else if (IsCmdEqual(Cmd, L"unload")) {
        Err = CDrvUnload();
    } else if (IsCmdEqual(Cmd, L"fltstart")) {
        Err = CDrvCtlFltStart();
    } else if (IsCmdEqual(Cmd, L"fltstop")) {
        Err = CDrvCtlFltStop();
    } else if (IsCmdEqual(Cmd, L"echo")) {
        Err = CDrvCtlEcho();
    } else if (IsCmdEqual(Cmd, L"bugcheck")) {
        Err = CDrvCtlBugCheck();
    } else if (IsCmdEqual(Cmd, L"test")) {
        Err = CDrvCtlTest();
    } else if (IsCmdEqual(Cmd, L"srvtime")) {
        if (argc != 3) {
            printf("Wrong num args, should be 2 args\n");
            Err = ERROR_INVALID_PARAMETER;
            goto exit;
        }
        Err = SrvPrintTime(argv[2]);
    } else {
        printf("Unknown cmd %ws\n", Cmd);
        Err = ERROR_INVALID_FUNCTION;
    }

exit:
    printf("ExitCode %d - 0x%x\n", Err, Err);
    TerminateProcess(GetCurrentProcess(), Err);

    return 0;
}

