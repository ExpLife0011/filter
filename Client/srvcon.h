#ifndef __FBACKUP_CLIENT_SRVCON_H__
#define __FBACKUP_CLIENT_SRVCON_H__

#include "base.h"

#include <winsock2.h>
#include <ws2tcpip.h>
#include <iphlpapi.h>

typedef struct _SRV_CON {
    SOCKET  Socket;
} SRV_CON, *PSRV_CON;

DWORD SrvConInit(VOID);
VOID SrvConRelease(VOID);

DWORD SrvConOpen(PWCHAR Host, PWCHAR Port, PSRV_CON *pSrvCon);
DWORD SrvConSend(PSRV_CON SrvCon, PVOID Buf, ULONG Size);
DWORD SrvConRecv(PSRV_CON SrvCon, PVOID Buf, ULONG Size, ULONG *pReceived, BOOL *pbClosed);
VOID SrvConClose(PSRV_CON SrvCon);

#endif