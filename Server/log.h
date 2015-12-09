#ifndef __FBACKUP_SERVER_LOG_H__
#define __FBACKUP_SERVER_LOG_H__

#include "base.h"

enum {
    LOG_DBG,
    LOG_INF,
    LOG_WRN,
    LOG_ERR
};

VOID GlobalLog(ULONG Level, PCHAR File, ULONG Line, PCHAR Func, PCHAR Fmt, ...);

#define LErr(Fmt, ...)   \
            GlobalLog(LOG_ERR, __FILE__, __LINE__, __FUNCTION__, (Fmt), ##__VA_ARGS__)

#define LDbg(Fmt, ...)   \
            GlobalLog(LOG_DBG, __FILE__, __LINE__, __FUNCTION__, (Fmt), ##__VA_ARGS__)

#define LInf(Fmt, ...)   \
            GlobalLog(LOG_INF, __FILE__, __LINE__, __FUNCTION__, (Fmt), ##__VA_ARGS__)

#define LWrn(Fmt, ...)   \
            GlobalLog(LOG_WRN, __FILE__, __LINE__, __FUNCTION__, (Fmt), ##__VA_ARGS__)

DWORD GlobalLogInit(PWCHAR FilePath, ULONG Level);
VOID GlobalLogRelease();

#endif