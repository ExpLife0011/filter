#include "inc\map.h"

#define MAP_ENTRY_KEY(MapEntry)     \
            (PVOID)((ULONG_PTR)(MapEntry) + FIELD_OFFSET(MAP_ENTRY, KeyValue))

#define MAP_ENTRY_VALUE(MapEntry)   \
            (PVOID)((ULONG_PTR)(MapEntry) + FIELD_OFFSET(MAP_ENTRY, KeyValue) + MapEntry->KeySize)

#define MAP_ENTRY_SIZE(MapEntry)    \
            (ULONG)(sizeof(*(MapEntry)) - sizeof(MapEntry->KeyValue) + MapEntry->KeySize + MapEntry->ValueSize)

PMAP_ENTRY MapEntryCreate(PVOID Key, ULONG KeySize, PVOID Value, ULONG ValueSize)
{
    PMAP_ENTRY MapEntry;
    
    MapEntry = ExAllocatePoolWithTag(NonPagedPool, sizeof(*MapEntry) - sizeof(MapEntry->KeyValue) + KeySize + ValueSize,
                                     MAP_TAG);
    if (!MapEntry)
        return NULL;

    MapEntry->KeySize = KeySize;
    MapEntry->ValueSize = ValueSize;
    RtlCopyMemory(MAP_ENTRY_KEY(MapEntry), Key, KeySize);
    if (ValueSize) {
        RtlCopyMemory(MAP_ENTRY_VALUE(MapEntry), Value, ValueSize);
    }

    return MapEntry;
}

VOID MapEntryFree(PMAP_ENTRY MapEntry)
{
    ExFreePoolWithTag(MapEntry, MAP_TAG);
}

RTL_GENERIC_COMPARE_RESULTS
MapAvlCmpRoutine(PRTL_AVL_TABLE Table, PMAP_ENTRY FirstStruct, PMAP_ENTRY  SecondStruct)
{
    ULONG Key1Size = FirstStruct->KeySize, Key2Size = SecondStruct->KeySize;
    PUCHAR pKey1, pKey2;
    UCHAR Key1, Key2;
    ULONG Index;

    if (Key1Size < Key2Size)
        return GenericLessThan;
    if (Key1Size > Key2Size)
        return GenericGreaterThan;

    pKey1 = MAP_ENTRY_KEY(FirstStruct);
    pKey2 = MAP_ENTRY_KEY(SecondStruct);

    for (Index = 0; Index < Key1Size; Index++) {
        Key1 = pKey1[Index];
        Key2 = pKey2[Index];
        if (Key1 > Key2)
            return GenericGreaterThan;
        if (Key1 < Key2)
            return GenericLessThan;
    }

    return GenericEqual;
}

PVOID MapAvlAllocRoutine(PRTL_AVL_TABLE Table, ULONG ByteSize)
{
    return ExAllocatePoolWithTag(NonPagedPool, ByteSize, MAP_TAG);
}

VOID MapAvlFreeRoutine(PRTL_AVL_TABLE  Table, PVOID  Buffer)
{
    ExFreePoolWithTag(Buffer, MAP_TAG);
}

VOID MapInit(PMAP Map)
{
    RtlInitializeGenericTableAvl(&Map->Avl, MapAvlCmpRoutine, MapAvlAllocRoutine, MapAvlFreeRoutine, NULL);
}

VOID MapRelease(PMAP Map)
{
    PMAP_ENTRY MapEntry;

    do {
        MapEntry = (PMAP_ENTRY)RtlEnumerateGenericTableAvl(&Map->Avl, TRUE);
        if (MapEntry) {
            RtlDeleteElementGenericTableAvl(&Map->Avl, MapEntry);
        }
    } while (MapEntry);
}

NTSTATUS MapInsert(PMAP Map, PVOID Key, ULONG KeySize, PVOID Value, ULONG ValueSize)
{
    PMAP_ENTRY MapEntry;
    BOOLEAN NewEntry;
    NTSTATUS Status;
    PVOID Result;
 
    if (!Key || !Value || !KeySize || !ValueSize)
        return STATUS_INVALID_PARAMETER;

    MapEntry = MapEntryCreate(Key, KeySize, Value, ValueSize);
    if (!MapEntry)
        return STATUS_INSUFFICIENT_RESOURCES;

    Result = RtlInsertElementGenericTableAvl(&Map->Avl, MapEntry, MAP_ENTRY_SIZE(MapEntry), &NewEntry);
    MapEntryFree(MapEntry);
    if (!Result) {
        Status = STATUS_INSUFFICIENT_RESOURCES;
    } else {
        if (!NewEntry)
            Status = STATUS_OBJECT_NAME_COLLISION;
        else
            Status = STATUS_SUCCESS;
    }

    return Status;
}

NTSTATUS MapSearch(PMAP Map, PVOID Key, ULONG KeySize, PVOID *pValue, ULONG *pValueSize)
{
    PMAP_ENTRY KeyEntry, FoundEntry;
    PVOID Value;

    KeyEntry = MapEntryCreate(Key, KeySize, NULL, 0);
    if (!KeyEntry)
        return STATUS_INSUFFICIENT_RESOURCES;

    FoundEntry = RtlLookupElementGenericTableAvl(&Map->Avl, KeyEntry);
    MapEntryFree(KeyEntry);
    if (!FoundEntry)
        return STATUS_OBJECT_NAME_NOT_FOUND;

    Value = ExAllocatePoolWithTag(NonPagedPool, FoundEntry->ValueSize, MAP_TAG);
    if (!Value)
        return STATUS_INSUFFICIENT_RESOURCES;

    RtlCopyMemory(Value, MAP_ENTRY_VALUE(FoundEntry), FoundEntry->ValueSize);
    *pValue = Value;
    *pValueSize = FoundEntry->ValueSize;
    return STATUS_SUCCESS;
}

NTSTATUS MapDelete(PMAP Map, PVOID Key, ULONG KeySize)
{
    PMAP_ENTRY KeyEntry;
    NTSTATUS Status;
    BOOLEAN Deleted;

    KeyEntry = MapEntryCreate(Key, KeySize, NULL, 0);
    if (!KeyEntry)
        return STATUS_INSUFFICIENT_RESOURCES;

    Deleted = RtlDeleteElementGenericTableAvl(&Map->Avl, KeyEntry);
    MapEntryFree(KeyEntry);

    if (Deleted)
        Status = STATUS_SUCCESS;
    else
        Status = STATUS_OBJECT_NAME_NOT_FOUND;

    return Status;
}