#ifndef __FBACKUP_SERVER_LOG_H__
#define __FBACKUP_SERVER_LOG_H__

#include "base.h"

enum {
    LOG_DBG,
    LOG_INF,
    LOG_WRN,
    LOG_ERR
};

VOID GlobalLog(ULONG Level, PCHAR Component, PCHAR File, PCHAR Func, ULONG Line, PCHAR Fmt, ...);

#define LErr(Fmt, ...)   \
            GlobalLog(LOG_ERR, __FBACKUP_COMPONENT__, __FILE__, __FUNCTION__, __LINE__, (Fmt), ##__VA_ARGS__)

#define LDbg(Fmt, ...)   \
            GlobalLog(LOG_DBG, __FBACKUP_COMPONENT__, __FILE__, __FUNCTION__, __LINE__, (Fmt), ##__VA_ARGS__)

#define LInf(Fmt, ...)   \
            GlobalLog(LOG_INF, __FBACKUP_COMPONENT__, __FILE__, __FUNCTION__, __LINE__, (Fmt), ##__VA_ARGS__)

#define LWrn(Fmt, ...)   \
            GlobalLog(LOG_WRN, __FBACKUP_COMPONENT__, __FILE__, __FUNCTION__, __LINE__, (Fmt), ##__VA_ARGS__)

DWORD GlobalLogInit(PWCHAR FilePath, ULONG Level);
VOID GlobalLogRelease();

#endif