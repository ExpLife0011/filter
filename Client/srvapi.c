#include "srvapi.h"

PSRVAPI_CONTEXT SrvApiConnect(PWCHAR Host, PWCHAR Port)
{
    return NULL;
}

DWORD SrvApiGetTime(PSRVAPI_CONTEXT Api, PSYSTEMTIME Time)
{
    return FB_E_UNIMPL;
}

VOID SrvApiRelease(PSRVAPI_CONTEXT Api)
{
    return;
}