#ifndef __FBACKUP_SERVER_TIME_H__
#define __FBACKUP_SERVER_TIME_H__

#include "base.h"

typedef struct _FBTIME {
    LARGE_INTEGER   Start;
    LARGE_INTEGER   Stop;
    LARGE_INTEGER   Freq;
    ULONG64         DeltaNs;
} FBTIME, *PFBTIME;

DWORD FbTimeStart(PFBTIME Time);
DWORD FbTimeStop(PFBTIME Time);

FORCEINLINE ULONG64 FbTimeDeltaNs(PFBTIME Time)
{
    return Time->DeltaNs;
}
#endif