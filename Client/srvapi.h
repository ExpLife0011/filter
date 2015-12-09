#ifndef __FBACKUP_CLIENT_SRVAPI_H__
#define __FBACKUP_CLIENT_SRVAPI_H__

#include "base.h"
#include "srvcon.h"

typedef struct _SRV_API_CTX {
    PSRV_CON    SrvCon;
} SRV_API_CTX, *PSRV_API_CTX;

DWORD SrvApiInit(VOID);
VOID SrvApiRelease(VOID);

DWORD SrvApiConnect(PWCHAR Host, PWCHAR Port, PSRV_API_CTX *pApiCtx);
DWORD SrvApiGetTime(PSRV_API_CTX ApiCtx, PSYSTEMTIME Time);
VOID SrvApiClose(PSRV_API_CTX ApiCtx);


#endif