#ifndef __FBACKUP_MAP_H__
#define __FBACKUP_MAP_H__

#include "inc\base.h"

#define MAP_TAG '0paM'

typedef struct _MAP_ENTRY {
    ULONG KeySize;
    ULONG ValueSize;
    PVOID Key;
    PVOID Value;
} MAP_ENTRY, *PMAP_ENTRY;

typedef struct _MAP {
    RTL_AVL_TABLE Avl;
    ERESOURCE Lock;
} MAP, *PMAP;

PMAP MapCreate(VOID);
VOID MapDelete(PMAP Map);

NTSTATUS MapInsertKey(PMAP Map, PVOID Key, ULONG KeySize, PVOID Value, ULONG ValueSize);
NTSTATUS MapLookupKey(PMAP Map, PVOID Key, ULONG KeySize, PVOID *pValue, ULONG *pValueSize);
NTSTATUS MapDeleteKey(PMAP Map, PVOID Key, ULONG KeySize);

NTSTATUS MapTest(VOID);

#endif
