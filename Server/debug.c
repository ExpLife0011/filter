#include "debug.h"
#include "fmt.h"
#include "misc.h"

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

PTOP_LEVEL_EXCEPTION_FILTER g_OldUnhandleExceptionFilter;
HMODULE g_ModuleHandle;

LONG DebugUnhandledExceptionFilter(EXCEPTION_POINTERS *ExceptionInfo)
{
    ULONG i;
    PEXCEPTION_RECORD ExceptionRecord = ExceptionInfo->ExceptionRecord;
    PCONTEXT Context = ExceptionInfo->ContextRecord;

    DebugPrintf("Exc Code 0x%x At %p ModuleBase %p",
                ExceptionRecord->ExceptionCode,
                ExceptionRecord->ExceptionAddress, g_ModuleHandle);

    for (i = 0; i < ULongMin(ExceptionRecord->NumberParameters,
                             RTL_NUMBER_OF(ExceptionRecord->ExceptionInformation)); i++)
        DebugPrintf("Exc Information[%d]=%p",
                    i, ExceptionRecord->ExceptionInformation[i]);

    DebugPrintf("Exc RAX %p RBX %p RDX %p RCX %p\n",
                Context->Rax, Context->Rbx, Context->Rdx, Context->Rcx);
    DebugPrintf("Exc RSI %p RDI %p RSP %p RBP %p\n",
                Context->Rsi, Context->Rdi, Context->Rsp, Context->Rbp);

    DebugPrintf("Exc R8 %p R9 %p  R10 %p R11 %p\n",
                Context->EFlags, Context->R8, Context->R9, Context->R10, Context->R11);

    DebugPrintf("Exc R12 %p R13 %p R14 %p R15 %p\n",
                Context->R12, Context->R13, Context->R14, Context->R15);

    DebugPrintf("Exc EFlags 0x%x\n", Context->EFlags);

    if (g_OldUnhandleExceptionFilter)
        return g_OldUnhandleExceptionFilter(ExceptionInfo);
    else
        return EXCEPTION_CONTINUE_SEARCH;
}

VOID DebugInit(VOID)
{
    g_ModuleHandle = GetModuleHandle(NULL);
    g_OldUnhandleExceptionFilter = SetUnhandledExceptionFilter(DebugUnhandledExceptionFilter);
}