#include "memalloc.h"
#include "debug.h"
#include "list_entry.h"
#include "spinlock.h"
#include "fmt.h"
#include "misc.h"

#define MEM_ENTRY_MAGIC 'gMeM'

#pragma pack(push, 1)
typedef struct _MEM_ENTRY {
    LIST_ENTRY  ListEntry;
    ULONG       Magic;
    ULONG       Size;
    PCHAR       Component;
    PCHAR       File;
    PCHAR       Function;
    ULONG       Line;
} MEM_ENTRY, *PMEM_ENTRY;
#pragma pack(pop)

#define MEM_CTX_HASH_BITS 10
#define MEM_CTX_HASH_SIZE (1 << MEM_CTX_HASH_BITS)

typedef struct _MEM_CTX {
    LIST_ENTRY  MemEntryList[MEM_CTX_HASH_SIZE];
    SPIN_LOCK   MemEntryListLock[MEM_CTX_HASH_SIZE];
} MEM_CTX, *PMEM_CTX;

PMEM_CTX g_MemCtx;

PMEM_CTX GetMemCtx()
{
    return g_MemCtx;
}

PVOID __MemAlloc(SIZE_T Size, PCHAR Component, PCHAR File, PCHAR Function, ULONG Line)
{
    PMEM_CTX MemCtx = GetMemCtx();
    PMEM_ENTRY MemEntry;
    ULONG Bucket;

    if (!MemCtx)
        return NULL;

    MemEntry = HeapAlloc(GetProcessHeap(), 0, sizeof(*MemEntry) + Size);
    if (!MemEntry)
        return NULL;

    MemEntry->Magic = MEM_ENTRY_MAGIC;
    MemEntry->Component = Component;
    MemEntry->File = File;
    MemEntry->Function = Function;
    MemEntry->Line = Line;
    MemEntry->Size = (ULONG)Size;

    Bucket = HashPtr(MemEntry) & (MEM_CTX_HASH_SIZE - 1);
    SpinLockLock(&MemCtx->MemEntryListLock[Bucket]);
    InsertHeadList(&MemCtx->MemEntryList[Bucket],
                   &MemEntry->ListEntry);
    SpinLockUnlock(&MemCtx->MemEntryListLock[Bucket]);

    return (PVOID)(MemEntry + 1);
}

VOID __MemFree(PVOID pMem)
{
    PMEM_CTX MemCtx = GetMemCtx();
    PMEM_ENTRY MemEntry = ((PMEM_ENTRY)pMem - 1);
    ULONG Bucket;

    if (!MemCtx) {
        DebugPrintf("No g_MemCtx!!!\n");
        return;
    }

    if (MemEntry->Magic != MEM_ENTRY_MAGIC) {
        DebugPrintf("MemEntry %p invalid magic 0x%x\n",
                    MemEntry, MemEntry->Magic);
        return;
    }

    Bucket = HashPtr(MemEntry) & (MEM_CTX_HASH_SIZE - 1);
    SpinLockLock(&MemCtx->MemEntryListLock[Bucket]);
    RemoveEntryList(&MemEntry->ListEntry);
    SpinLockUnlock(&MemCtx->MemEntryListLock[Bucket]);

    HeapFree(GetProcessHeap(), 0, MemEntry);
}

DWORD MemAllocInit(VOID)
{
    PMEM_CTX MemCtx;
    ULONG i;

    MemCtx = HeapAlloc(GetProcessHeap(), 0, sizeof(*MemCtx));
    if (!MemCtx) {
        DebugPrintf("No memory to alloc MemCtx\n");
        return FB_E_NO_MEMORY;
    }

    for (i = 0; i < RTL_NUMBER_OF(MemCtx->MemEntryList); i++) {
        InitializeListHead(&MemCtx->MemEntryList[i]);
        SpinLockInit(&MemCtx->MemEntryListLock[i]);
    }

    g_MemCtx = MemCtx;
    return 0;
}

VOID MemAllocRelease(VOID)
{
    PMEM_CTX MemCtx = g_MemCtx;
    LIST_ENTRY MemEntryList;
    PLIST_ENTRY ListEntry;
    PMEM_ENTRY MemEntry;
    ULONG i;

    g_MemCtx = NULL;
    if (!MemCtx) {
        DebugPrintf("No MemCtx\n");
        return;
    }

    for (i = 0; i < RTL_NUMBER_OF(MemCtx->MemEntryList); i++) {
        SpinLockLock(&MemCtx->MemEntryListLock[i]);
        MoveList(&MemEntryList, &MemCtx->MemEntryList[i]);
        SpinLockUnlock(&MemCtx->MemEntryListLock[i]);

        for (ListEntry = MemEntryList.Flink; ListEntry != &MemEntryList;
             ListEntry = ListEntry->Flink) {
            MemEntry = CONTAINING_RECORD(ListEntry, MEM_ENTRY, ListEntry);
            DebugPrintf("Memory leak %p size %d allocated at %s:%s():%s,%d\n",
                        MemEntry, MemEntry->Size, MemEntry->Component,
                        MemEntry->Function, FmtTruncatePath(MemEntry->File),
                        MemEntry->Line);
        }
    }
    HeapFree(GetProcessHeap(), 0, MemCtx);
}

