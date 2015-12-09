#include "server.h"
#include "log.h"
#include "list_entry.h"
#include "api.h"

#include <winsock2.h>
#include <ws2tcpip.h>
#include <iphlpapi.h>

typedef struct _FBSERVER FBSERVER, *PFBSERVER;
typedef struct _FBWORKER FBWORKER, *PFBWORKER;
typedef struct _FBCLIENT FBCLIENT, *PFBCLIENT;

enum {
    FBCLIENT_IO_RECEIVE,
    FBCLIENT_IO_SEND
};

typedef struct _FBCLIENT_IO {
    LIST_ENTRY      ListEntry;
    PFBCLIENT       Client;
    ULONG           Size;
    DWORD           Err;
    ULONG           Operation;
    WSAOVERLAPPED   Overlapped;
    WSABUF          Buf;
} FBCLIENT_IO, *PFBCLIENT_IO;

enum {
    FBCLIENT_S_CREATED,
    FBCLIENT_S_RECV_HEADER,
    FBCLIENT_S_RECV_BODY,
    FBCLIENT_S_SEND_HEADER,
    FBCLIENT_S_SEND_BODY
};

typedef struct _FBCLIENT {
    LIST_ENTRY          ListEntry;
    SOCKET              Socket;
    CRITICAL_SECTION    Lock;
    PFBSERVER           Server;
    LIST_ENTRY          IoListHead;
    ULONG               State;
    FB_SRV_REQ_HEADER   ReqHeader;
    FB_SRV_RESP_HEADER  RespHeader;
    PVOID               ReqBody;
    PVOID               RespBody;
} FBCLIENT, *PFBCLIENT;

typedef struct _FBSERVER {
    HANDLE              hIoCompPort;
    PFBWORKER           *Worker;
    ULONG               NumWorkers;
    WSADATA             WsaData;
    SOCKET              ListenSocket;
    HANDLE              hAcceptThread;
    volatile LONG       Stopping;
    PADDRINFOW          AddrInfo;
    CRITICAL_SECTION    Lock;
    LIST_ENTRY          ClientListHead;
} FBSERVER, *PFBSERVER;

typedef struct _FBWORKER {
    HANDLE hThread;
    volatile LONG Stopping;
    PVOID   Context;
} FBWORKER, *PFBWORKER;

FBSERVER g_FbServer;

PFBSERVER GetFbServer(VOID)
{
    return &g_FbServer;
}

VOID ClientIoFree(PFBCLIENT_IO Io)
{
    free(Io);
}

VOID ClientIoDelete(PFBCLIENT_IO Io)
{
    PFBCLIENT Client = Io->Client;

    EnterCriticalSection(&Client->Lock);
    RemoveEntryList(&Io->ListEntry);
    LeaveCriticalSection(&Client->Lock);
    ClientIoFree(Io);
}

PFBCLIENT_IO ClientIoCreate(PFBCLIENT Client, ULONG Operation, PVOID Buf, ULONG Size)
{
    PFBCLIENT_IO Io;

    Io = malloc(sizeof(*Io));
    if (!Io)
        return NULL;

    memset(Io, 0, sizeof(*Io));
    Io->Client = Client;
    Io->Operation = Operation;
    Io->Size = Size;
    Io->Buf.buf = (PCHAR)Buf;
    Io->Buf.len = Size;

    EnterCriticalSection(&Client->Lock);
    InsertTailList(&Client->IoListHead, &Io->ListEntry);
    LeaveCriticalSection(&Client->Lock);
    return Io;
}

DWORD ClientIoExec(PFBCLIENT_IO ClientIo)
{
    PFBCLIENT Client = ClientIo->Client;
    DWORD Flags;
    int Result;
    DWORD Err;

    switch (ClientIo->Operation) {
    case FBCLIENT_IO_RECEIVE:
        Flags = 0;
        Result = WSARecv(Client->Socket, &ClientIo->Buf, 1, NULL, &Flags, &ClientIo->Overlapped, NULL);
        break;
    case FBCLIENT_IO_SEND:
        Flags = 0;
        Result = WSASend(Client->Socket, &ClientIo->Buf, 1, NULL, Flags, &ClientIo->Overlapped, NULL);
        break;
    default:
        LErr("Unknown operation %d", ClientIo->Operation);
        return ERROR_INVALID_PARAMETER;
    }

    if (Result) {
        Err = WSAGetLastError();
        if (Err != WSA_IO_PENDING)
            LErr("WSARecv err %d", Err);
        else
            Err = 0;
    } else
        Err = 0;

    return Err;
}

DWORD ClientIoQueue(PFBCLIENT Client, ULONG Operation, PVOID Buf, ULONG Size)
{
    PFBCLIENT_IO ClientIo;
    DWORD Err;

    ClientIo = ClientIoCreate(Client, Operation, Buf, Size);
    if (!ClientIo) {
        return FB_E_NO_MEMORY;
    }

    Err = ClientIoExec(ClientIo);
    if (Err) {
        ClientIoDelete(ClientIo);
    }

    return Err;
}

VOID ClientIosDelete(PFBCLIENT Client)
{
    LIST_ENTRY IoListHead;
    PLIST_ENTRY ListEntry;
    PFBCLIENT_IO Io;

    EnterCriticalSection(&Client->Lock);
    MoveList(&IoListHead, &Client->IoListHead);
    LeaveCriticalSection(&Client->Lock);

    while (!IsListEmpty(&IoListHead)) {
        ListEntry = RemoveHeadList(&IoListHead);
        Io = CONTAINING_RECORD(ListEntry, FBCLIENT_IO, ListEntry);
        ClientIoFree(Io);
    }
}

VOID ServerWorkerStop(PFBWORKER Worker, PFBSERVER Server)
{
    Worker->Stopping = 1;
    WaitForSingleObject(Worker->hThread, INFINITE);
    CloseHandle(Worker->hThread);
}

VOID ClientFree(PFBCLIENT Client)
{
    DeleteCriticalSection(&Client->Lock);
    if (Client->ReqBody)
        free(Client->ReqBody);
    if (Client->RespBody)
        free(Client->RespBody);
    free(Client);
}

VOID ClientClose(PFBCLIENT Client)
{
    EnterCriticalSection(&Client->Lock);
    if (Client->Socket != INVALID_SOCKET) {
        shutdown(Client->Socket, SD_BOTH);
        closesocket(Client->Socket);
        Client->Socket = INVALID_SOCKET;
    }
    LeaveCriticalSection(&Client->Lock);
    ClientIosDelete(Client);
}

VOID ClientDelete(PFBCLIENT Client)
{
    PFBSERVER Server = Client->Server;

    EnterCriticalSection(&Server->Lock);
    RemoveEntryList(&Client->ListEntry);
    LeaveCriticalSection(&Server->Lock);

    Client->Server = NULL;
    ClientClose(Client);
    ClientFree(Client);
}

DWORD ServerTimeRequest(PVOID *pRespBody, ULONG *RespBodySize)
{
    PFB_SRV_RESP_TIME pTime;
    SYSTEMTIME Time;

    pTime = malloc(sizeof(*pTime));
    if (!pTime)
        return FB_E_NO_MEMORY;

    GetSystemTime(&Time);
    pTime->Year = Time.wYear;
    pTime->Month = Time.wMonth;
    pTime->DayOfWeek = Time.wDayOfWeek;
    pTime->Day = Time.wDay;
    pTime->Hour = Time.wHour;
    pTime->Minute = Time.wMinute;
    pTime->Second = Time.wSecond;
    pTime->Milliseconds = Time.wMilliseconds;

    *pRespBody = pTime;
    *RespBodySize = sizeof(*pTime);

    return 0;
}

DWORD ServerHandleRequest(PFB_SRV_REQ_HEADER ReqHeader, PVOID ReqBody,
                          PFB_SRV_RESP_HEADER RespHeader, PVOID *pRespBody)
{
    DWORD Err;
    ULONG RespBodySize = 0;

    *pRespBody = NULL;
    switch (ReqHeader->Type) {
    case FB_SRV_REQ_TYPE_TIME:
        Err = ServerTimeRequest(pRespBody, &RespBodySize);
        break;
    default:
        LErr("Unknown request type %d", ReqHeader->Type);
        Err = FB_E_UNK_REQUEST;
        break;
    }

    RespHeader->Magic = FB_SRV_RESP_MAGIC;
    RespHeader->Err = Err;
    RespHeader->Size = RespBodySize;
    RespHeader->Type = ReqHeader->Type;
    RespHeader->Id = ReqHeader->Id;

    return Err;
}

VOID ServerWorkerHandleIo(PFBCLIENT Client, PFBCLIENT_IO ClientIo, ULONG Size, DWORD Err)
{
    LDbg("Client %p State %d Size %d ClientIo %p Op %d Err %d",
         Client, Client->State, Size, ClientIo, ClientIo->Operation, Err);

    if (Err)
        goto fail_client;

    if (ClientIo->Size != Size) {
        LErr("Client %p ClientIo %p Size %d vs. Received %d",
             Client, ClientIo, ClientIo->Size, Size);
        goto fail_client;
    }

    ClientIoDelete(ClientIo);

restart:
    switch (Client->State) {
    case FBCLIENT_S_RECV_HEADER: {
        PFB_SRV_REQ_HEADER ReqHeader = &Client->ReqHeader;

        if (ReqHeader->Magic != FB_SRV_REQ_MAGIC) {
            LErr("Invalid client request magic 0x%x", ReqHeader->Magic);
            goto fail_client;
            return;
        }

        if (ReqHeader->Type <= FB_SRV_REQ_TYPE_INVALID ||
            ReqHeader->Type >= FB_SRV_REQ_TYPE_MAX) {
            LErr("Invalid client request type %d", ReqHeader->Type);
            goto fail_client;
        }

        if (ReqHeader->Size > FB_SRV_MAX_BODY_SIZE) {
            LErr("Invalid client request size %d", ReqHeader->Size);
            goto fail_client;
        }

        if (ReqHeader->Size) {
            Client->ReqBody = malloc(ReqHeader->Size);
            if (!Client->ReqBody) {
                LErr("No memory");
                goto fail_client;
            }

            Client->State = FBCLIENT_S_RECV_BODY;
            Err = ClientIoQueue(Client, FBCLIENT_IO_RECEIVE, Client->ReqBody, ReqHeader->Size);
            if (Err)
                goto fail_client;
        } else {
            Client->State = FBCLIENT_S_RECV_BODY;
            goto restart;
        }
        break;
    }
    case FBCLIENT_S_RECV_BODY:
        Err = ServerHandleRequest(&Client->ReqHeader, Client->ReqBody,
                                  &Client->RespHeader, &Client->RespBody);
        if (Err)
            goto fail_client;

        if (Client->ReqBody) {
            free(Client->ReqBody);
            Client->ReqBody = NULL;
        }
        Client->State = FBCLIENT_S_SEND_HEADER;
        Err = ClientIoQueue(Client, FBCLIENT_IO_SEND, &Client->RespHeader, sizeof(Client->RespHeader));
        if (Err)
            goto fail_client;
        break;
    case FBCLIENT_S_SEND_HEADER:
        if (Client->RespHeader.Size == 0) {
            Client->State = FBCLIENT_S_CREATED;
            break;
        }

        Client->State = FBCLIENT_S_SEND_BODY;
        Err = ClientIoQueue(Client, FBCLIENT_IO_SEND, Client->RespBody, Client->RespHeader.Size);
        if (Err)
            goto fail_client;
        break;
    case FBCLIENT_S_SEND_BODY:
        if (Client->RespBody) {
            free(Client->RespBody);
            Client->RespBody = NULL;
        }
        Client->State = FBCLIENT_S_CREATED;
        break;
    default:
        LErr("Unknown client %p state %d", Client, Client->State);
        goto fail_client;
    }

    return;

fail_client:
    ClientDelete(Client);
    return;
}

DWORD ServerWorkerRoutine(PFBWORKER Worker)
{
    PFBSERVER Server = (PFBSERVER)Worker->Context;
    DWORD IoBytes;
    ULONG_PTR CompKey;
    OVERLAPPED *pOverlapped;
    DWORD Err;
    PFBCLIENT Client;
    PFBCLIENT_IO ClientIo;

    LInf("Worker starting");

    while (!Worker->Stopping) {
        if (!GetQueuedCompletionStatus(Server->hIoCompPort, &IoBytes,
                                       &CompKey, &pOverlapped, 1000)) {
            Err = GetLastError();
        } else
            Err = 0;

        if (Worker->Stopping)
            break;

        if (Err == WAIT_TIMEOUT)
            continue;

        Client = (PFBCLIENT)CompKey;
        ClientIo = CONTAINING_RECORD(pOverlapped, FBCLIENT_IO, Overlapped);
        if (Err)
            LErr("GetQueuedCompletionStatus failed Error %d", Err);

        ServerWorkerHandleIo(Client, ClientIo, IoBytes, Err);
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
        return FB_E_NO_MEMORY;

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

    memset(&Hints, 0, sizeof(Hints));

    Hints.ai_family = AF_INET;
    Hints.ai_socktype = SOCK_STREAM;
    Hints.ai_protocol = IPPROTO_TCP;
    Hints.ai_flags = AI_PASSIVE;

    if (GetAddrInfoW(NULL, FB_SRV_PORT, &Hints, &AddrInfo)) {
        Err = WSAGetLastError();
        LErr("GetAddrInfoW failed Error %d", Err);
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

PFBCLIENT ClientCreate(PFBSERVER Server, SOCKET Socket)
{
    PFBCLIENT Client;
    BOOL Inserted;

    if (Server->Stopping)
        return NULL;

    Client = malloc(sizeof(*Client));
    if (!Client)
        return NULL;

    memset(Client, 0, sizeof(*Client));
    InitializeListHead(&Client->IoListHead);
    InitializeCriticalSection(&Client->Lock);
    Client->Socket = Socket;
    EnterCriticalSection(&Server->Lock);
    if (!Server->Stopping) {
        Client->Server = Server;
        Client->State = FBCLIENT_S_CREATED;
        InsertTailList(&Server->ClientListHead, &Client->ListEntry);
        Inserted = TRUE;
    } else
        Inserted = FALSE;
    LeaveCriticalSection(&Server->Lock);

    if (!Inserted) {
        Client->Socket = INVALID_SOCKET;
        ClientFree(Client);
        Client = NULL;
    }

    return Client;
}

DWORD ServerAcceptRoutine(PFBSERVER Server)
{
    DWORD Err;
    SOCKET Socket;
    PFBCLIENT Client;

    while (!Server->Stopping) {
        LDbg("Start accepting");
        Socket = accept(Server->ListenSocket, NULL, NULL);
        if (Socket == INVALID_SOCKET) {
            Err = WSAGetLastError();
            LErr("Accept Error %d", Err);
            continue;
        }
        LDbg("Accept succeded");
        Client = ClientCreate(Server, Socket);
        if (!Client) {
            if (!Server->Stopping)
                LErr("Can't create client");
            else
                LWrn("Can't insert client - server stopping");
            continue;
        }

        if (!CreateIoCompletionPort((HANDLE)Socket, Server->hIoCompPort,
                                    (ULONG_PTR)Client, 0)) {
            Err = GetLastError();
            LErr("CreateIoCompletionPort failed Error %d", Err);
            ClientDelete(Client);
        }
        LDbg("Client %p attached to IO completion port", Client);
        Client->State = FBCLIENT_S_RECV_HEADER;
        Err = ClientIoQueue(Client, FBCLIENT_IO_RECEIVE,
                            &Client->ReqHeader, sizeof(Client->ReqHeader));
        if (Err) {
            LErr("ClientIoQueue failed Error %d", Err);
            ClientDelete(Client);
        }
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

    if (GetAddrInfoW(L"127.0.0.1", FB_SRV_PORT, &Hints, &AddrInfo)) {
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
    InitializeListHead(&Server->ClientListHead);
    InitializeCriticalSection(&Server->Lock);

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
    DeleteCriticalSection(&Server->Lock);
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

VOID ServerDeleteClients(PFBSERVER Server)
{
    PLIST_ENTRY ListEntry;
    LIST_ENTRY ClientListHead;
    PFBCLIENT Client;

    LInf("Moving clients to delete");
    EnterCriticalSection(&Server->Lock);
    MoveList(&ClientListHead, &Server->ClientListHead);
    LeaveCriticalSection(&Server->Lock);
    LInf("Deleting clients");
    while (!IsListEmpty(&ClientListHead)) {
        ListEntry = RemoveHeadList(&ClientListHead);
        Client = CONTAINING_RECORD(ListEntry, FBCLIENT, ListEntry);
        ClientClose(Client);
        ClientFree(Client);
    }
    LInf("Clients deleted");
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
    ServerDeleteClients(Server);

    if (WSACleanup()) {
        LErr("WSACleanup failed Error %d", WSAGetLastError());
    }
    
    DeleteCriticalSection(&Server->Lock);
    LInf("Resources released");
    LInf("Server stopped");
}