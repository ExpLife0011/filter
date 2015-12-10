#ifndef __FBACKUP_SERVER_DEBUG_H__
#define __FBACKUP_SERVER_DEBUG_H__

#include "base.h"

VOID __DebugPrintf(PCHAR Component, PCHAR File, PCHAR Function, ULONG Line, PCHAR Fmt, ...);

#define DebugPrintf(Fmt, ...) \
            __DebugPrintf(__FBACKUP_COMPONENT__, __FILE__, __FUNCTION__, __LINE__, (Fmt), ##__VA_ARGS__);

#endif