#ifndef __FBACKUP_MAP_H__
#define __FBACKUP_MAP_H__

#include "inc\base.h"

#define MAP_TAG '0paM'

typedef struct _MAP_ENTRY {
    ULONG KeySize;
    ULONG ValueSize;
    UCHAR KeyValue[1];
} MAP_ENTRY, *PMAP_ENTRY;

typedef struct _MAP {
    RTL_AVL_TABLE Avl;
} MAP, *PMAP;

VOID MapInit(PMAP Map);
VOID MapRelease(PMAP Map);
NTSTATUS MapInsert(PMAP Map, PVOID Key, ULONG KeySize, PVOID Value, ULONG ValueSize);
NTSTATUS MapSearch(PMAP Map, PVOID Key, ULONG KeySize, PVOID *pValue, ULONG *pValueSize);
NTSTATUS MapDelete(PMAP Map, PVOID Key, ULONG KeySize);

#endif
