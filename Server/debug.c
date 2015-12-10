#include "debug.h"
#include "fmt.h"

VOID __DebugPrintf(PCHAR Component, PCHAR File, PCHAR Function, ULONG Line, PCHAR Fmt, ...)
{
    va_list Args;
    CHAR Output[256];
    ULONG BufLeft;
    PCHAR BufPos;

    BufPos = Output;
    BufLeft = RTL_NUMBER_OF(Output);

    FmtMsg(&BufPos, &BufLeft, "%s:%s():%s,%d: ",
           Component, Function, FmtTruncatePath(File), Line);

    va_start(Args, Fmt);
    FmtMsg2(&BufPos, &BufLeft, Fmt, Args);
    va_end(Args);

    Output[RTL_NUMBER_OF(Output)-1] = '\0';
    OutputDebugStringA(Output);
}