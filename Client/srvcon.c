#include "srvcon.h"

DWORD SrvOpen(PWCHAR Host, PWCHAR Port, PSRV_CON *pSrvCon)
{
    SOCKET Socket;
    DWORD Err;
    ADDRINFOW Hints, *AddrInfo;
    PSRV_CON SrvCon;

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
        printf("GetAddrInfoW failed Error %d", Err);
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

    if (connect(Socket, AddrInfo->ai_addr, (int)AddrInfo->ai_addrlen)) {
        Err = WSAGetLastError();
        printf("socket connect failed Error %d", Err);
        FreeAddrInfoW(AddrInfo);
        free(SrvCon);
        return Err;
    }

    *pSrvCon = SrvCon;
    return 0;
}

DWORD SrvSend(PSRV_CON SrvCon, PVOID Buf, ULONG Size)
{
    int Sent;
    ULONG TotalSent = 0;

    while (TotalSent < Size) {
        Sent = send(SrvCon->Socket, (PCHAR)Buf + TotalSent,
                    Size - TotalSent, 0);
        if (Sent == SOCKET_ERROR) {
            return WSAGetLastError();
        }
        TotalSent+= Sent;
    }

    return 0;
}

DWORD SrvRecv(PSRV_CON SrvCon, PVOID Buf, ULONG Size, ULONG *pReceived, BOOL *pbClosed)
{
    int Received;
    ULONG TotalReceived = 0;

    while (TotalReceived < Size) {
        Received = recv(SrvCon->Socket, (PCHAR)Buf + TotalReceived,
                        Size - TotalReceived, 0);
        if (Received == SOCKET_ERROR) {
            return WSAGetLastError();
        }
        if (Received == 0) {
            *pbClosed = TRUE;
            return 0;
        }
        TotalReceived+= Received;
    }

    return 0;
}

VOID SrvClose(PSRV_CON SrvCon)
{
    shutdown(SrvCon->Socket, SD_BOTH);
    closesocket(SrvCon->Socket);
    free(SrvCon);
}
