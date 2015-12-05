#include "inc\map.h"

RTL_GENERIC_COMPARE_RESULTS
MapAvlCmpRoutine(PRTL_AVL_TABLE Table, PMAP_ENTRY Entry1, PMAP_ENTRY  Entry2)
{
    ULONG Key1Size = Entry1->KeySize, Key2Size = Entry2->KeySize;
    PUCHAR pKey1, pKey2;
    UCHAR Key1, Key2;
    ULONG Index;

    if (Key1Size < Key2Size)
        return GenericLessThan;
    if (Key1Size > Key2Size)
        return GenericGreaterThan;

    pKey1 = Entry1->Key;
    pKey2 = Entry2->Key;

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
    PMAP_ENTRY Entry = (PMAP_ENTRY)((ULONG_PTR)Buffer + sizeof(RTL_BALANCED_LINKS));

    ExFreePoolWithTag(Entry->Key, MAP_TAG);
    ExFreePoolWithTag(Entry->Value, MAP_TAG);

    ExFreePoolWithTag(Buffer, MAP_TAG);
}

VOID MapInit(PMAP Map)
{
    RtlInitializeGenericTableAvl(&Map->Avl, MapAvlCmpRoutine, MapAvlAllocRoutine, MapAvlFreeRoutine, NULL);
}

VOID MapRelease(PMAP Map)
{
    PMAP_ENTRY Entry;

    do {
        Entry = (PMAP_ENTRY)RtlEnumerateGenericTableAvl(&Map->Avl, TRUE);
        if (Entry) {
            RtlDeleteElementGenericTableAvl(&Map->Avl, Entry);
        }
    } while (Entry);
}

NTSTATUS MapInsert(PMAP Map, PVOID Key, ULONG KeySize, PVOID Value, ULONG ValueSize)
{
    MAP_ENTRY Entry;
    BOOLEAN NewEntry;
    NTSTATUS Status;
    PVOID Result;
 
    if (!Key || !Value || !KeySize || !ValueSize)
        return STATUS_INVALID_PARAMETER;

    Entry.Key = ExAllocatePoolWithTag(NonPagedPool, KeySize, MAP_TAG);
    if (!Entry.Key)
        return STATUS_INSUFFICIENT_RESOURCES;
    Entry.Value = ExAllocatePoolWithTag(NonPagedPool, ValueSize, MAP_TAG);
    if (!Entry.Value) {
        ExFreePoolWithTag(Entry.Key, MAP_TAG);
        return STATUS_INSUFFICIENT_RESOURCES;
    }
    RtlCopyMemory(Entry.Key, Key, KeySize);
    RtlCopyMemory(Entry.Value, Value, ValueSize);
    Entry.KeySize = KeySize;
    Entry.ValueSize = ValueSize;

    Result = RtlInsertElementGenericTableAvl(&Map->Avl, &Entry, sizeof(Entry), &NewEntry);
    if (!Result) {
        Status = STATUS_INSUFFICIENT_RESOURCES;
    } else {
        if (!NewEntry)
            Status = STATUS_OBJECT_NAME_COLLISION;
        else
            Status = STATUS_SUCCESS;
    }

    if (!NT_SUCCESS(Status)) {
        ExFreePoolWithTag(Entry.Key, MAP_TAG);
        ExFreePoolWithTag(Entry.Value, MAP_TAG);
    }

    return Status;
}

NTSTATUS MapSearch(PMAP Map, PVOID Key, ULONG KeySize, PVOID *pValue, ULONG *pValueSize)
{
    MAP_ENTRY Entry;
    PMAP_ENTRY FoundEntry;
    PVOID Value;

    Entry.Key = Key;
    Entry.KeySize = KeySize;
    Entry.Value = NULL;
    Entry.ValueSize = 0;
    
    FoundEntry = RtlLookupElementGenericTableAvl(&Map->Avl, &Entry);
    if (!FoundEntry)
        return STATUS_OBJECT_NAME_NOT_FOUND;

    Value = ExAllocatePoolWithTag(NonPagedPool, FoundEntry->ValueSize, MAP_TAG);
    if (!Value)
        return STATUS_INSUFFICIENT_RESOURCES;

    RtlCopyMemory(Value, FoundEntry->Value, FoundEntry->ValueSize);
    *pValue = Value;
    *pValueSize = FoundEntry->ValueSize;
    return STATUS_SUCCESS;
}

NTSTATUS MapDelete(PMAP Map, PVOID Key, ULONG KeySize)
{
    MAP_ENTRY Entry;
    NTSTATUS Status;
    BOOLEAN Deleted;

    Entry.Key = Key;
    Entry.KeySize = KeySize;
    Entry.Value = NULL;
    Entry.ValueSize = 0;

    Deleted = RtlDeleteElementGenericTableAvl(&Map->Avl, &Entry);
    if (Deleted)
        Status = STATUS_SUCCESS;
    else
        Status = STATUS_OBJECT_NAME_NOT_FOUND;

    return Status;
}

typedef struct _SID_KV {
    CHAR *Key;
    CHAR *Value;
} SID_KV, *PSID_KV;

NTSTATUS MapTest(VOID)
{
    NTSTATUS Status;
    PVOID Value;
    MAP Map;
    ULONG Index, ValueSize;
    SID_KV TestKvs[] = {{"S-1-0-0", "SID0"},
                        {"S-1-1-0", "SID1"},
                        {"S-1-0", "SID2"},
                        {"S-1-1", "SID3"},
                        {"S-1-5-21-1180699209-877415012-3182924384-1004", "BIG-SID4"},
                        {"S-1-5-20", "SID5"},
                        {"S-1-5-21-1180699209-111115012-3182924384-1004", "Another-BIG-SID6"}};
    MapInit(&Map);
    for (Index = 0; Index < RTL_NUMBER_OF(TestKvs); Index++) {
        Status = MapInsert(&Map, TestKvs[Index].Key, (ULONG)(strlen(TestKvs[Index].Key) + 1),
                                 TestKvs[Index].Value, (ULONG)(strlen(TestKvs[Index].Value) + 1));
        if (!NT_SUCCESS(Status)) {
            DbgPrint("MapInsert failed Status 0x%x\n", Status);
            goto cleanup;
        }
    }

    /* Try to insert key 4 again */
    Status = MapInsert(&Map, TestKvs[4].Key, (ULONG)(strlen(TestKvs[4].Key) + 1),
                             TestKvs[4].Value, (ULONG)(strlen(TestKvs[4].Value) + 1));
    if (Status != STATUS_OBJECT_NAME_COLLISION) {
        Status = STATUS_UNSUCCESSFUL;
        DbgPrint("MapInsert duplicate Status 0x%x\n", Status);
        goto cleanup;
    }

    /* Delete key 5 */
    Status = MapDelete(&Map, TestKvs[5].Key, (ULONG)(strlen(TestKvs[5].Key) + 1));
    if (!NT_SUCCESS(Status)) {
        DbgPrint("MapDelete Status 0x%x\n", Status);
        goto cleanup;
    }
    
    for (Index = 0; Index < RTL_NUMBER_OF(TestKvs); Index++) {
        /* Skip key 5 because it's already deleted */
        if (Index == 5)
            continue;
        Status = MapSearch(&Map, TestKvs[Index].Key, (ULONG)(strlen(TestKvs[Index].Key) + 1), &Value, &ValueSize);
        if (!NT_SUCCESS(Status)) {
            DbgPrint("MapSearch failed Status 0x%x\n", Status);
            goto cleanup;
        }

        if (ValueSize != (strlen(TestKvs[Index].Value) + 1)) {
            DbgPrint("MapSearch returned invalid ValueSize 0x%x\n", ValueSize);
            ExFreePoolWithTag(Value, MAP_TAG);
            Status = STATUS_UNSUCCESSFUL;
            goto cleanup;
        }
        if (ValueSize != RtlCompareMemory(Value, TestKvs[Index].Value, ValueSize)) {
            DbgPrint("MapSearch returned invalid Value content\n");
            ExFreePoolWithTag(Value, MAP_TAG);
            Status = STATUS_UNSUCCESSFUL;
            goto cleanup;            
        }
        ExFreePoolWithTag(Value, MAP_TAG);
    }
    Status = STATUS_SUCCESS;
cleanup:
    DbgPrint("MapTest Status 0x%x\n", Status);
    MapRelease(&Map);
    return Status;
}