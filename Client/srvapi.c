#include "srvapi.h"
#include "..\server\api.h"

DWORD SrvApiConnect(PWCHAR Host, PWCHAR Port, PSRV_API_CTX *pApiCtx)
{
    PSRV_API_CTX ApiCtx;
    DWORD Err;

    ApiCtx = malloc(sizeof(*ApiCtx));
    if (!ApiCtx)
        return FB_E_NO_MEMORY;

    memset(ApiCtx, 0, sizeof(*ApiCtx));
    Err = SrvConOpen(Host, Port, &ApiCtx->SrvCon);
    if (Err) {
        free(ApiCtx);
        return Err;
    }

    *pApiCtx = ApiCtx;
    return 0;
}

DWORD SrvApiRecvResponse(PSRV_API_CTX ApiCtx, PVOID RespBody, ULONG RespBodySize)
{
    ULONG Received;
    FB_SRV_RESP_HEADER RespHeader;
    DWORD Err;

    Err = SrvConRecv(ApiCtx->SrvCon, &RespHeader, sizeof(RespHeader), &Received, NULL);
    if (Err)
        return Err;

    if (Received != sizeof(RespHeader))
        return FB_E_INCOMP_HEADER;

    if (RespHeader.Magic != FB_SRV_RESP_MAGIC)
        return FB_E_BAD_MAGIC;

    Err = RespHeader.Err;
    if (Err)
        return Err;

    if (RespBodySize != RespHeader.Size)
        return FB_E_BAD_SIZE;

    Err = SrvConRecv(ApiCtx->SrvCon, RespBody, RespBodySize, &Received, NULL);
    if (Err)
        return Err;

    if (RespBodySize != Received)
        return FB_E_INCOMP_DATA;

    return 0;
}

DWORD SrvApiSendRequest(PSRV_API_CTX ApiCtx, ULONG Type, PVOID Body, ULONG BodySize)
{
    FB_SRV_REQ_HEADER ReqHeader;
    DWORD Err;

    ReqHeader.Magic = FB_SRV_REQ_MAGIC;
    ReqHeader.Type = Type;
    ReqHeader.Id = 0;
    ReqHeader.Size = BodySize;

    Err = SrvConSend(ApiCtx->SrvCon, &ReqHeader, sizeof(ReqHeader));
    if (Err)
        return Err;

    if (BodySize)
        Err = SrvConSend(ApiCtx->SrvCon, Body, BodySize);

    return Err;
}

DWORD SrvApiGetTime(PSRV_API_CTX ApiCtx, PSYSTEMTIME pTime)
{
    FB_SRV_RESP_TIME Time;
    DWORD Err;

    Err = SrvApiSendRequest(ApiCtx, FB_SRV_REQ_TYPE_TIME, NULL, 0);
    if (Err)
        return Err;

    Err = SrvApiRecvResponse(ApiCtx, &Time, sizeof(Time));
    if (Err)
        return Err;

    pTime->wYear = Time.Year;
    pTime->wMonth = Time.Month;
    pTime->wDayOfWeek = Time.DayOfWeek;
    pTime->wDay = Time.Day;
    pTime->wHour = Time.Hour;
    pTime->wMinute = Time.Minute;
    pTime->wSecond = Time.Second;
    pTime->wMilliseconds = Time.Milliseconds;

    return 0;
}

VOID SrvApiClose(PSRV_API_CTX ApiCtx)
{
    SrvConClose(ApiCtx->SrvCon);
    free(ApiCtx);
    return;
}

DWORD SrvApiInit(VOID)
{
    return SrvConInit();
}

VOID SrvApiRelease(VOID)
{
    SrvConRelease();
}