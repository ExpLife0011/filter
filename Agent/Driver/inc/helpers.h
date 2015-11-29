#ifndef __FBACKUP_HELPERS_H__
#define __FBACKUP_HELPERS_H__

#include "inc\base.h"

#ifdef DBG
#define DPRINT DbgPrint
#define DEBUG_BREAK() __debugbreak()
#define DEBUG_BREAK_ON(cond)        \
{                                   \
    if (cond) {                     \
            DEBUG_BREAK();          \
    }                               \
}
#else
#define DPRINT
#define DEBUG_BREAK()
#define DEBUG_BREAK_ON(cond)
#endif

#define ntohs(x) RtlUshortByteSwap(x)
#define htons(x) RtlUshortByteSwap(x)

#define ntohl(x) RtlUlongByteSwap(x)
#define htonl(x) RtlUlongByteSwap(x)

#define ntohll(x) RtlUlonglongByteSwap(x)
#define htonll(x) RtlUlonglongByteSwap(x)

#define PTR_BY_RVA(Base, Rva)   ((PVOID)((ULONG_PTR)(Base) + (ULONG_PTR)(Rva)))

#define ARRAY_SIZE(arr) (sizeof(arr)/sizeof((arr)[0]))

FORCEINLINE PVOID NpAlloc(SIZE_T Size, ULONG Tag)
{
    return ExAllocatePoolWithTag(NonPagedPool, Size, Tag);
}

FORCEINLINE VOID NpFree(PVOID Addr, ULONG Tag)
{
    ExFreePoolWithTag(Addr, Tag);
}

FORCEINLINE PVOID PpAlloc(SIZE_T Size, ULONG Tag)
{
    return ExAllocatePoolWithTag(PagedPool, Size, Tag);
}

FORCEINLINE VOID PpFree(PVOID Addr, ULONG Tag)
{
    ExFreePoolWithTag(Addr, Tag);
}

FORCEINLINE LONG64 MillisTo100Ns(ULONG Millis)
{
        return 10*1000*((LONG64)Millis);
}

FORCEINLINE VOID ThreadSleepMs(ULONG Millis)
{
    LARGE_INTEGER Interval;

    Interval.QuadPart = -MillisTo100Ns(Millis);
    KeDelayExecutionThread(KernelMode, FALSE, &Interval);
}

NTSTATUS GetObjectName(PVOID Object, PUNICODE_STRING *pObjectName);
NTSTATUS GetObjectByName(PUNICODE_STRING ObjName, POBJECT_TYPE ObjectType, PVOID *pObject);

#endif