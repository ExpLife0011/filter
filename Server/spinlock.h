#ifndef __FBACKUP_SERVER_SPINLOCK_H__
#define __FBACKUP_SERVER_SPINLOCK_H__

#include "base.h"

typedef struct _SPIN_LOCK {
    volatile LONG   Value;
    DWORD           Owner;
} SPIN_LOCK, *PSPIN_LOCK;

FORCEINLINE VOID SpinLockInit(PSPIN_LOCK Lock)
{
    Lock->Value = 0;
}

FORCEINLINE VOID SpinLockLock(PSPIN_LOCK Lock)
{
    while (1) {
        if (0 == InterlockedCompareExchange(&Lock->Value, 1, 0))
            break;

        _mm_pause();
    }
    Lock->Owner = GetCurrentThreadId();
}

FORCEINLINE VOID SpinLockUnlock(PSPIN_LOCK Lock)
{
    Lock->Owner = 0;
    InterlockedCompareExchange(&Lock->Value, 0, 1);
}

#endif