#include "fmt.h"

LONG FmtMsg2(PCHAR *pBuff, ULONG *pLeft, PCHAR Fmt, va_list Args)
{
    LONG Result;

    if (*pLeft < 0)
        return -1;

    Result = _vsnprintf(*pBuff, *pLeft, Fmt, Args);
    if (Result) {
        *pBuff+=Result;
        *pLeft-=Result;
    } 

    return Result;
}

LONG FmtMsg(PCHAR *pBuff, ULONG *pLeft, PCHAR Fmt, ...)
{
    LONG Result;
    va_list Args;

    va_start(Args, Fmt);
    Result = FmtMsg2(pBuff, pLeft, Fmt, Args);
    va_end(Args);

    return Result;
}

PCHAR FmtTruncatePath(PCHAR FileName)
{
    PCHAR BaseName;

    BaseName = strrchr(FileName, '\\');
    if (BaseName)
        return ++BaseName;
    else
        return FileName;
}