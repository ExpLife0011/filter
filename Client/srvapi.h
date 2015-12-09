#ifndef __FBACKUP_CLIENT_SRVAPI_H__
#define __FBACKUP_CLIENT_SRVAPI_H__

#include "base.h"
#include "srvcon.h"

typedef struct _SRVAPI_CONTEXT {
    PSRV_CON    SrvCon;
} SRVAPI_CONTEXT, *PSRVAPI_CONTEXT;

PSRVAPI_CONTEXT SrvApiConnect(PWCHAR Host, PWCHAR Port);
DWORD SrvApiGetTime(PSRVAPI_CONTEXT Api, PSYSTEMTIME Time);
VOID SrvApiRelease(PSRVAPI_CONTEXT Api);

#endif