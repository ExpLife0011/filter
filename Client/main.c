#include "client.h"
#include "srvapi.h"
#include "..\server\api.h"

typedef struct _SRV_LOAD_TEST_CTX {
    HANDLE  hThread;
    DWORD   Err;
    ULONG   NumReqs;
    PWCHAR  Host;
} SRV_LOAD_TEST_CTX, *PSRV_LOAD_TEST_CTX;

DWORD SrvLoadTestWork(PWCHAR Host)
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

api_close:
    SrvApiClose(ApiCtx);
    SrvApiRelease();
    return Err;
}

DWORD SrvLoadTestThread(PSRV_LOAD_TEST_CTX LoadTestCtx)
{
    ULONG i;
    DWORD Err;

    for (i = 0; i < LoadTestCtx->NumReqs; i++) {
        Err = SrvLoadTestWork(LoadTestCtx->Host);
        if (Err)
            goto ret;
    }
    Err = 0;
ret:
    LoadTestCtx->Err = Err;
    return Err;
}

DWORD SrvLoadTest(PWCHAR Host, ULONG NumTreads, ULONG NumReqs)
{
    DWORD Err;
    PSRV_LOAD_TEST_CTX *LoadTestCtx, pLoadTestCtx;
    ULONG i, j;
    ULONG ReqsPerThread;

    printf("SrvLoadTest params: Host %ws, NumTreads %d, NumReqs %d\n",
           Host, NumTreads, NumReqs);

    if (!Host || !NumReqs || !NumTreads)
        return FB_E_INVAL;

    LoadTestCtx = (PSRV_LOAD_TEST_CTX *)malloc(NumTreads*sizeof(*LoadTestCtx));
    if (!LoadTestCtx)
        return FB_E_NO_MEMORY;

    ReqsPerThread = NumReqs/NumTreads;
    memset(LoadTestCtx, 0, NumTreads*sizeof(*LoadTestCtx));
    for (i = 0; i < NumTreads; i++) {
        pLoadTestCtx = (PSRV_LOAD_TEST_CTX)malloc(sizeof(*LoadTestCtx[i]));
        if (!pLoadTestCtx) {
            Err = FB_E_NO_MEMORY;
            goto fail_free_load_ctx;
        }
        LoadTestCtx[i] = pLoadTestCtx;
        memset(pLoadTestCtx, 0, sizeof(*pLoadTestCtx));
        pLoadTestCtx->NumReqs = ReqsPerThread;
        pLoadTestCtx->Host = Host;
        pLoadTestCtx->hThread = CreateThread(NULL, 0, SrvLoadTestThread, pLoadTestCtx,
                                             CREATE_SUSPENDED, NULL);
        if (!pLoadTestCtx->hThread) {
            Err = GetLastError();
            goto fail_stop_load_ctx;
        }
    }

    for (i = 0; i < NumTreads; i++) {
        pLoadTestCtx = LoadTestCtx[i];
        ResumeThread(pLoadTestCtx->hThread);
    }

    for (i = 0; i < NumTreads; i++) {
        pLoadTestCtx = LoadTestCtx[i];
        WaitForSingleObject(pLoadTestCtx->hThread, INFINITE);
        CloseHandle(pLoadTestCtx->hThread);
    }

    Err = 0;
    for (i = 0; i < NumTreads; i++) {
        pLoadTestCtx = LoadTestCtx[i];
        if (pLoadTestCtx->Err)
            Err = pLoadTestCtx->Err;
    }

    for (i = 0; i < NumTreads; i++)
        free(LoadTestCtx[i]);

    free(LoadTestCtx);
    return Err;

fail_stop_load_ctx:
    for (j = 0; j < i; j++) {
        pLoadTestCtx = LoadTestCtx[i];
        WaitForSingleObject(pLoadTestCtx->hThread, INFINITE);
        CloseHandle(pLoadTestCtx->hThread);
    }
fail_free_load_ctx:
    for (j = 0; j < i; j++)
        free(LoadTestCtx[j]);

    free(LoadTestCtx);
    return Err;
}

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
    } else if (IsCmdEqual(Cmd, L"srvloadtest")) {
        if (argc != 5) {
            printf("Wrong num args, should be 4 args\n");
            Err = ERROR_INVALID_PARAMETER;
            goto exit;
        }
        Err = SrvLoadTest(argv[2], _wtoi(argv[3]), _wtoi(argv[4]));
    } else {
        printf("Unknown cmd %ws\n", Cmd);
        Err = ERROR_INVALID_FUNCTION;
    }

exit:
    printf("ExitCode %d - 0x%x\n", Err, Err);
    TerminateProcess(GetCurrentProcess(), Err);

    return 0;
}

