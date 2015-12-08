#include "server.h"
#include "log.h"
#include "error.h"

#include <winsock2.h>
#include <ws2tcpip.h>
#include <iphlpapi.h>

#define SERVER_PORT L"9111"

typedef struct _FBWORKER {
    HANDLE hThread;
    volatile LONG Stopping;
    PVOID   Context;
} FBWORKER, *PFBWORKER;

typedef struct _FBSERVER {
    HANDLE          hIoCompPort;
    PFBWORKER       *Worker;
    ULONG           NumWorkers;
    WSADATA         WsaData;
    SOCKET          ListenSocket;
    HANDLE          hAcceptThread;
    volatile LONG   Stopping;
    PADDRINFOW      AddrInfo;
} FBSERVER, *PFBSERVER;

FBSERVER g_FbServer;

PFBSERVER GetFbServer(VOID)
{
    return &g_FbServer;
}

VOID ServerWorkerStop(PFBWORKER Worker, PFBSERVER Server)
{
    Worker->Stopping = 1;
    WaitForSingleObject(Worker->hThread, INFINITE);
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

DWORD ServerBind(PFBSERVER Server)
{
    SOCKET ListenSocket;
    DWORD Err;
    ADDRINFOW Hints, *AddrInfo;

    ListenSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (ListenSocket == INVALID_SOCKET) {
        Err = WSAGetLastError();
        LErr("Bind failed Error %d", Err);
        return Err;
    }
    
    memset(&Hints, 0, sizeof(Hints));

    Hints.ai_family = AF_INET;
    Hints.ai_socktype = SOCK_STREAM;
    Hints.ai_protocol = IPPROTO_TCP;
    Hints.ai_flags = AI_PASSIVE;

    if (GetAddrInfoW(NULL, SERVER_PORT, &Hints, &AddrInfo)) {
        Err = WSAGetLastError();
        LErr("GetAddrInfoW failed Error %d", Err);
        closesocket(ListenSocket);
        return Err;
    }

    ListenSocket = socket(AddrInfo->ai_family, AddrInfo->ai_socktype, AddrInfo->ai_protocol);
    if (ListenSocket == INVALID_SOCKET) {
        Err = WSAGetLastError();
        LErr("socket failed Error %d", Err);
        FreeAddrInfoW(AddrInfo);
        return Err;
    }

    if (bind(ListenSocket, AddrInfo->ai_addr, (int)AddrInfo->ai_addrlen)) {
        Err = WSAGetLastError();
        LErr("bind failed Error %d", Err);
        FreeAddrInfoW(AddrInfo);
        closesocket(ListenSocket);
        return Err;
    }
    FreeAddrInfoW(AddrInfo);

    Server->ListenSocket = ListenSocket;
    LInf("Binded");
    return 0;
}

DWORD ServerAcceptRoutine(PFBSERVER Server)
{
    DWORD Err;
    SOCKET Socket;

    while (!Server->Stopping) {
        LInf("Start accepting");
        Socket = accept(Server->ListenSocket, NULL, NULL);
        if (Socket == INVALID_SOCKET) {
            Err = WSAGetLastError();
            LErr("Accept Error %d", Err);
        } else {
            LInf("Accept succeded");
            shutdown(Socket, SD_BOTH);
            closesocket(Socket);
        }
        if (Server->Stopping)
            break;
    }
    LInf("Accept thread stopped");
    return 0;
}

DWORD LocalServerConnect(SOCKET *pSocket)
{
    SOCKET Socket;
    DWORD Err;
    ADDRINFOW Hints, *AddrInfo;

    memset(&Hints, 0, sizeof(Hints));

    Hints.ai_family = AF_INET;
    Hints.ai_socktype = SOCK_STREAM;
    Hints.ai_protocol = IPPROTO_TCP;
    Hints.ai_flags = AI_PASSIVE;

    if (GetAddrInfoW(L"127.0.0.1", SERVER_PORT, &Hints, &AddrInfo)) {
        Err = WSAGetLastError();
        LErr("GetAddrInfoW failed Error %d", Err);
        return Err;
    }

    Socket = socket(AddrInfo->ai_family, AddrInfo->ai_socktype, AddrInfo->ai_protocol);
    if (Socket == INVALID_SOCKET) {
        Err = WSAGetLastError();
        return Err;
    }

    if (connect(Socket, AddrInfo->ai_addr, (int)AddrInfo->ai_addrlen)) {
        Err = WSAGetLastError();
        LErr("connect failed Error %d", Err);
        closesocket(Socket);
        return Err;
    }

    *pSocket = Socket;
    return 0;
}

DWORD ServerStart(VOID)
{
    PFBSERVER Server = GetFbServer();
    DWORD Err;

    LInf("Server starting");
    memset(Server, 0, sizeof(*Server));

    Err = WSAStartup(WINSOCK_VERSION, &Server->WsaData);
    if (Err) {
        LErr("WSAStartup failed Error %d", Err);
        Server->Stopping = 1;
        goto fail;
    }

    LInf("WSA version 0x%x", Server->WsaData.wVersion);

    Err = ServerBind(Server);
    if (Err) {
        Server->Stopping = 1;
        goto fail_wsa_cleanup;
    }

    if (listen(Server->ListenSocket, SOMAXCONN)) {
        Err = WSAGetLastError();
        LErr("listen Error %d", Err);
        goto fail_close_listen_socket;
    }

    Server->hAcceptThread = CreateThread(NULL, 0, ServerAcceptRoutine, Server, 0, NULL);
    if (!Server->hAcceptThread) {
        Err = GetLastError();
        Server->Stopping = 1;
        goto fail_close_listen_socket;
    }

    Server->hIoCompPort = CreateIoCompletionPort(INVALID_HANDLE_VALUE, NULL, 0, 0); 
    if (!Server->hIoCompPort) {
        Err = GetLastError();
        Server->Stopping = 1;
        goto fail_delete_accept_thread;
    }

    Err = ServerCreateWorkers(Server, 4);
    if (Err) {
        LErr("ServerCreateWorkers Error %d", Err);
        Server->Stopping = 1;
        goto fail_close_io_comp_port;
    }

    Err = NO_ERROR;
    LInf("Server start Err %d", Err);
    return Err;
fail_close_io_comp_port:
    CloseHandle(Server->hIoCompPort);
fail_delete_accept_thread:
    WaitForSingleObject(Server->hAcceptThread, INFINITE);
    CloseHandle(Server->hAcceptThread);
fail_close_listen_socket:
    closesocket(Server->ListenSocket);
fail_wsa_cleanup:
    if (WSACleanup()) {
        LErr("WSACleanup failed Error %d", WSAGetLastError());
    }
fail:
    LErr("Server start Error %d", Err);
    return Err;
}

VOID ServerStopAcceptThread(PFBSERVER Server)
{
    SOCKET Socket;

    /* Kick accept by connect */
    if (0 == LocalServerConnect(&Socket)) {
        closesocket(Socket);
    }

    WaitForSingleObject(Server->hAcceptThread, INFINITE);
    CloseHandle(Server->hAcceptThread);
}

VOID ServerStop(VOID)
{
    PFBSERVER Server = GetFbServer();

    LInf("Server stopping");

    Server->Stopping = 1;
    ServerStopAcceptThread(Server);
    closesocket(Server->ListenSocket);

    ServerDeleteWorkers(Server);
    LInf("Workers stopped");

    CloseHandle(Server->hIoCompPort);

    if (WSACleanup()) {
        LErr("WSACleanup failed Error %d", WSAGetLastError());
    }
    LInf("Resources released");
    LInf("Server stopped");
}