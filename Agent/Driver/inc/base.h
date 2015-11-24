#ifndef __FBACKUP_BASE_H__
#define __FBACKUP_BASE_H__

#include <ntifs.h>
#include <wsk.h>
#include <Ntstrsafe.h>
#include <ntimage.h>

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

FORCEINLINE PVOID NpAlloc(SIZE_T Size, ULONG Tag) {
    return ExAllocatePoolWithTag(NonPagedPool, Size, Tag);
}

FORCEINLINE VOID NpFree(PVOID Addr, ULONG Tag) {
    ExFreePoolWithTag(Addr, Tag);
}

FORCEINLINE PVOID PpAlloc(SIZE_T Size, ULONG Tag) {
    return ExAllocatePoolWithTag(PagedPool, Size, Tag);
}

FORCEINLINE VOID PpFree(PVOID Addr, ULONG Tag) {
    ExFreePoolWithTag(Addr, Tag);
}

#endif
