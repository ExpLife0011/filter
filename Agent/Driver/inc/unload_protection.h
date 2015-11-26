#ifndef __FBACKUP_UNLOAD_PROTECTION_H__
#define __FBACKUP_UNLOAD_PROTECTION_H__

#include "inc\base.h"
#include "inc\helpers.h"

typedef struct _UNLOAD_PROTECTION {
    volatile LONG RefCount;
} UNLOAD_PROTECTION, *PUNLOAD_PROTECTION;

FORCEINLINE VOID UnloadProtectionCheck(PUNLOAD_PROTECTION Protection)
{
    if (Protection->RefCount < 0)
        __debugbreak();
}

FORCEINLINE VOID UnloadProtectionInit(PUNLOAD_PROTECTION Protection)
{
    Protection->RefCount = 1;
}

FORCEINLINE VOID UnloadProtectionAcquire(PUNLOAD_PROTECTION Protection)
{
    UnloadProtectionCheck(Protection);
    InterlockedIncrement(&Protection->RefCount);
}

FORCEINLINE VOID UnloadProtectionRelease(PUNLOAD_PROTECTION Protection)
{
    UnloadProtectionCheck(Protection);
    InterlockedDecrement(&Protection->RefCount);
    UnloadProtectionCheck(Protection);
}

FORCEINLINE VOID UnloadProtectionWait(PUNLOAD_PROTECTION Protection)
{
    UnloadProtectionCheck(Protection);
    while (Protection->RefCount != 1) {
        ThreadSleepMs(1000);
    }

    InterlockedDecrement(&Protection->RefCount);
    ThreadSleepMs(1000);
    if (Protection->RefCount != 0)
        __debugbreak();
}

#endif