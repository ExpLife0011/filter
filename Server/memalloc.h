#ifndef __FBACKUP_SERVER_MEM_ALLOC_H__
#define __FBACKUP_SERVER_MEM_ALLOC_H__

#include "base.h"

DWORD MemAllocInit(VOID);
VOID MemAllocRelease(VOID);

PVOID __MemAlloc(SIZE_T Size, PCHAR Component, PCHAR File, PCHAR Function, ULONG Line);
VOID __MemFree(PVOID pMem);

#define MemAlloc(Size)  \
            __MemAlloc((Size), __FBACKUP_COMPONENT__, __FILE__, __FUNCTION__, __LINE__)

#define MemFree(pMem)   \
            __MemFree((pMem))

#endif