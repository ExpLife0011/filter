#ifndef __FBACKUP_SERVER_MISC_H__
#define __FBACKUP_SERVER_MISC_H__

#include "base.h"

FORCEINLINE ULONG ULongMin(ULONG Val1, ULONG Val2)
{
    return (Val1 < Val2) ? Val1 : Val2;
}

FORCEINLINE ULONG HashPtr(PVOID Ptr)
{
    ULONG_PTR Val = (ULONG_PTR)Ptr;
    ULONG Hash;
    ULONG i;
    UCHAR c;

    Hash = 5381;
    Val = Val >> 3; /* ignore lower 3 bits (usually these bits = 0)*/
    for (i = 0; i < sizeof(Val); i++) {
        c = Val & 0xFF;
        Hash = ((Hash << 5) + Hash) + c;
        Val = Val >> 8;
    }

    return Hash;
}

#endif