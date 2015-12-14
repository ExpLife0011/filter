#ifndef __FBACKUP_SERVER_BTREE_H__
#define __FBACKUP_SERVER_BTREE_H__

#include "base.h"

typedef struct _BTreeNode BTreeNode, *PBTreeNode;

typedef struct _BTreeCallbacks {
    ULONG (*BTreeNodeRead)(PVOID Ctx, ULONG64 Position, PBTreeNode Node);
    ULONG (*BTreeNodeWrite)(PVOID Ctx, PBTreeNode Node, ULONG_PTR TxId);
    ULONG (*BTreeWriteBegin)(PVOID Ctx, ULONG_PTR *pTxId);
    ULONG (*BTreeWriteEnd)(PVOID Ctx, ULONG_PTR TxId);
    LONG (*BTreeKeyCmp)(PVOID Key1, PVOID Key2);
    PVOID (*BTreeValueCopy)(PVOID Value);
} BTreeCallbacks, *PBTreeCallbacks;

typedef struct _BTree {
    PBTreeNode Root;
    PVOID Ctx;
    BTreeCallbacks Callbacks;
} BTree, *PBTree;

typedef struct _BTreeNode {
    ULONG64 Position;
    PVOID *Keys;
    ULONG NrKeys;
    PVOID *Values;
    ULONG NrValues;
    ULONG T;
} BTreeNode, *PBTreeNode;

ULONG BTreeInit(PBTree Tree, ULONG64 RootPosition, PVOID Ctx, PBTreeCallbacks Callbacks);

ULONG BTreeInsert(PBTree Tree, PVOID Key, PVOID Value);

ULONG BTreeLookup(PBTree Tree, PVOID Key, PVOID *pValue);

ULONG BTreeDelete(PBTree Tree, PVOID Key);

ULONG BTreeErase(PBTree Tree);

typedef ULONG (*BTreeEnumClb)(PVOID Key, PVOID Value);

ULONG BTreeEnum(PBTree Tree, BTreeEnumClb EnumClb);

ULONG BTreeRelease(PBTree Tree);

#endif