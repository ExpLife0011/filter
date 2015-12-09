#ifndef __FBACKUP_CLIENT_SRVCON_H__
#define __FBACKUP_CLIENT_SRVCON_H__

#include "base.h"

#include <winsock2.h>
#include <ws2tcpip.h>
#include <iphlpapi.h>

typedef struct _SRV_CON {
    SOCKET  Socket;
} SRV_CON, *PSRV_CON;

DWORD SrvOpen(PWCHAR Host, PWCHAR Port, PSRV_CON *pSrvCon);

DWORD SrvSend(PSRV_CON SrvCon, PVOID Buf, ULONG Size);

DWORD SrvRecv(PSRV_CON SrvCon, PVOID Buf, ULONG Size, ULONG *pReceived, BOOL *pbClosed);

VOID SrvClose(PSRV_CON SrvCon);

#endif