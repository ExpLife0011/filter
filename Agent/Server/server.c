#include "server.h"
#include "log.h"
#include "error.h"

typedef struct _FBWORKER {
    HANDLE hThread;
    volatile Stopping;
    PVOID   Context;
} FBWORKER, *PFBWORKER;

typedef struct _FBSERVER {
    HANDLE      hIoCompPort;
    PFBWORKER   *Worker;
    ULONG       NumWorkers;
} FBSERVER, *PFBSERVER;

FBSERVER g_FbServer;

PFBSERVER GetFbServer(VOID)
{
    return &g_FbServer;
}

VOID ServerWorkerStop(PFBWORKER Worker, PFBSERVER Server)
{
    Worker->Stopping = 1;
    LInf("Waiting thread");
    WaitForSingleObject(Worker->hThread, INFINITE);
    LInf("Waited thread");
    CloseHandle(Worker->hThread);
}

DWORD ServerWorkerRoutine(PFBWORKER Worker)
{
    PFBSERVER Server = (PFBSERVER)Worker->Context;
    DWORD IoBytes;
    ULONG_PTR CompKey;
    OVERLAPPED *pOverlapped;
    DWORD Err;

    LInf("Worker starting");

    while (!Worker->Stopping) {
        if (!GetQueuedCompletionStatus(Server->hIoCompPort, &IoBytes,
                                       &CompKey, &pOverlapped, 1000)) {
            Err = GetLastError();
            if (Err != WAIT_TIMEOUT)
                LErr("GetQueuedCompletionStatus failed Error %d", Err);
        }
        if (Worker->Stopping)
            break;
    }

    LInf("Worker stopped");
    return 0;
}

DWORD ServerWorkerStart(PFBWORKER Worker, PFBSERVER Server)
{
    memset(Worker, 0, sizeof(*Worker));
    Worker->Context = Server;
    Worker->hThread = CreateThread(NULL, 0, ServerWorkerRoutine, Worker, 0, NULL);
    if (!Worker->hThread)
        return GetLastError();

    return 0;
}

DWORD ServerCreateWorkers(PFBSERVER Server, ULONG NumWorkers)
{
    ULONG i, j;
    DWORD Err;

    Server->NumWorkers = 0;
    Server->Worker = (PFBWORKER *)malloc(NumWorkers*sizeof(PFBWORKER *));
    if (!Server->Worker)
        return FB_ERROR_NO_MEMORY;

    memset(Server->Worker, 0, NumWorkers*sizeof(PFBWORKER *));
    for (i = 0; i < NumWorkers; i++) {
        Server->Worker[i] = (PFBWORKER)malloc(sizeof(FBWORKER));
        if (!Server->Worker[i]) {
            Err = GetLastError();
            goto fail;
        }

        Err = ServerWorkerStart(Server->Worker[i], Server);
        if (Err) {
            free(Server->Worker[i]);
            goto fail;
        }
    }

    Server->NumWorkers = NumWorkers;
    return 0;

fail:
    for (j = 0; j < i; j++)
        Server->Worker[j]->Stopping = 1;

    for (j = 0; j < i; j++) {
        ServerWorkerStop(Server->Worker[i], Server);
        free(Server->Worker[i]);
    }
    free(Server->Worker);
    Server->Worker = NULL;

    return Err;
}

VOID ServerDeleteWorkers(PFBSERVER Server)
{
    ULONG i;

    for (i = 0; i < Server->NumWorkers; i++)
        Server->Worker[i]->Stopping = 1;

    for (i = 0; i < Server->NumWorkers; i++) {
        ServerWorkerStop(Server->Worker[i], Server);
        free(Server->Worker[i]);
    }
    free(Server->Worker);
    Server->Worker = NULL;
    Server->NumWorkers = 0;
}

DWORD ServerStart(VOID)
{
    PFBSERVER Server = GetFbServer();
    DWORD Err;

    LInf("Server starting");
    memset(Server, 0, sizeof(*Server));

    Server->hIoCompPort = CreateIoCompletionPort(INVALID_HANDLE_VALUE, NULL, 0, 0); 
    if (!Server->hIoCompPort) {
        Err = GetLastError();
        goto fail;
    }

    Err = ServerCreateWorkers(Server, 4);
    if (Err) {
        LErr("ServerCreateWorkers Error %d", Err);
        goto fail_close_io_comp_port;
    }

    Err = NO_ERROR;
    LInf("Server start Err %d", Err);
    return Err;

fail_close_io_comp_port:
    CloseHandle(Server->hIoCompPort);
fail:
    LErr("Server start Error %d", Err);
    return Err;
}

VOID ServerStop(VOID)
{
    PFBSERVER Server = GetFbServer();

    LInf("Server stopping");
    ServerDeleteWorkers(Server);
    LInf("Workers stopped");
    CloseHandle(Server->hIoCompPort);
    LInf("Resources released");
    LInf("Server stopped");
}