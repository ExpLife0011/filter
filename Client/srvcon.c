#include "srvcon.h"

DWORD SrvConOpen(PWCHAR Host, PWCHAR Port, PSRV_CON *pSrvCon)
{
    SOCKET Socket;
    DWORD Err;
    ADDRINFOW Hints, *AddrInfo;
    PSRV_CON SrvCon;
    int OptVal;

    SrvCon = malloc(sizeof(*SrvCon));
    if (!SrvCon)
        return FB_E_NO_MEMORY;

    memset(SrvCon, 0, sizeof(*SrvCon));
    memset(&Hints, 0, sizeof(Hints));

    Hints.ai_family = AF_INET;
    Hints.ai_socktype = SOCK_STREAM;
    Hints.ai_protocol = IPPROTO_TCP;
    Hints.ai_flags = AI_PASSIVE;

    if (GetAddrInfoW(Host, Port, &Hints, &AddrInfo)) {
        Err = WSAGetLastError();
        printf("GetAddrInfoW failed Error %d\n", Err);
        free(SrvCon);
        return Err;
    }

    Socket = socket(AddrInfo->ai_family, AddrInfo->ai_socktype,
                    AddrInfo->ai_protocol);
    if (Socket == INVALID_SOCKET) {
        Err = WSAGetLastError();
        FreeAddrInfoW(AddrInfo);
        free(SrvCon);
        return Err;
    }

    OptVal = 1;
    if (setsockopt(Socket, SOL_SOCKET, SO_REUSEADDR,
                    (char *) &OptVal, sizeof (OptVal))) {
        Err = WSAGetLastError();
        printf("socket connect failed Error %d\n", Err);
        FreeAddrInfoW(AddrInfo);
        closesocket(Socket);
        free(SrvCon);
        return Err;
    }

    if (connect(Socket, AddrInfo->ai_addr, (int)AddrInfo->ai_addrlen)) {
        Err = WSAGetLastError();
        printf("socket connect failed Error %d\n", Err);
        FreeAddrInfoW(AddrInfo);
        closesocket(Socket);
        free(SrvCon);
        return Err;
    }

    SrvCon->Socket = Socket;
    *pSrvCon = SrvCon;
    return 0;
}

DWORD SrvConSend(PSRV_CON SrvCon, PVOID Buf, ULONG Size)
{
    int Sent;
    ULONG TotalSent = 0;

    while (TotalSent < Size) {
        Sent = send(SrvCon->Socket, (PCHAR)Buf + TotalSent,
                    Size - TotalSent, 0);
        if (Sent == SOCKET_ERROR)
            return WSAGetLastError();

        TotalSent+= Sent;
    }

    return 0;
}

DWORD SrvConRecv(PSRV_CON SrvCon, PVOID Buf, ULONG Size, ULONG *pReceived, BOOL *pbClosed)
{
    int Received;
    ULONG TotalReceived = 0;

    while (TotalReceived < Size) {
        Received = recv(SrvCon->Socket, (PCHAR)Buf + TotalReceived,
                        Size - TotalReceived, 0);
        if (Received == SOCKET_ERROR)
            return WSAGetLastError();

        if (Received == 0) {
            if (pbClosed)
                *pbClosed = TRUE;
            break;
        }
        TotalReceived+= Received;
    }

    *pReceived = TotalReceived;
    return 0;
}

VOID SrvConClose(PSRV_CON SrvCon)
{
    shutdown(SrvCon->Socket, SD_BOTH);
    closesocket(SrvCon->Socket);
    free(SrvCon);
}

DWORD SrvConInit(VOID)
{
    WSADATA WsaData;

    return WSAStartup(WINSOCK_VERSION, &WsaData);
}

VOID SrvConRelease(VOID)
{
    WSACleanup();
}
