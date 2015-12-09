#include "log.h"
#include "list_entry.h"

typedef struct _LOG_ENTRY {
    CHAR        Buf[256];
    ULONG       BufUsed;
    LIST_ENTRY  ListEntry;
} LOG_ENTRY, *PLOG_ENTRY;

typedef struct _LOG_CONTEXT {
    HANDLE              hThread;
    HANDLE              hFile;
    HANDLE              hEvent;
    WCHAR               FilePath[256];
    LIST_ENTRY          ListHead;
    CRITICAL_SECTION    Lock;
    volatile LONG       Stopping;
    ULONG               Level;
} LOG_CONTEXT, *PLOG_CONTEXT;

LOG_CONTEXT g_LogCtx;

LONG WriteMsg2(PCHAR *pBuff, ULONG *pLeft, PCHAR Fmt, va_list Args)
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

LONG WriteMsg(PCHAR *pBuff, ULONG *pLeft, PCHAR Fmt, ...)
{
    LONG Result;
    va_list Args;

    va_start(Args, Fmt);
    Result = WriteMsg2(pBuff, pLeft, Fmt, Args);
    va_end(Args);

    return Result;
}

PCHAR TruncatePath(PCHAR FileName)
{
    PCHAR BaseName;

    BaseName = strrchr(FileName, '\\');
    if (BaseName)
        return ++BaseName;
    else
        return FileName;
}

DWORD LogFileSetPointerToEnd(PLOG_CONTEXT LogCtx)
{
    LARGE_INTEGER Offset;

    Offset.QuadPart = 0;
    if (!SetFilePointerEx(LogCtx->hFile, Offset, NULL, FILE_END))
        return GetLastError();
    return 0;
}

DWORD LogFileLock(PLOG_CONTEXT LogCtx)
{
    OVERLAPPED Overlapped;

    memset(&Overlapped, 0, sizeof(Overlapped));
    if (!LockFileEx(LogCtx->hFile, LOCKFILE_EXCLUSIVE_LOCK, 0, 0xFFFFFFFF, 0xFFFFFFFF, &Overlapped))
        return GetLastError();

    return 0;
}

VOID LogFileUnlock(PLOG_CONTEXT LogCtx)
{
    OVERLAPPED Overlapped;

    memset(&Overlapped, 0, sizeof(Overlapped));
    UnlockFileEx(LogCtx->hFile, 0, 0xFFFFFFFF, 0xFFFFFFFF, &Overlapped);
}

DWORD LogFileWriteEntry(PLOG_CONTEXT LogCtx, PLOG_ENTRY LogEntry)
{
    DWORD WrittenTotal = 0;
    DWORD Written;

    while (WrittenTotal < LogEntry->BufUsed) {
        if (!WriteFile(LogCtx->hFile, LogEntry->Buf + WrittenTotal,
                       LogEntry->BufUsed - WrittenTotal, &Written, NULL)) {
            return GetLastError();
        }
        WrittenTotal+= Written;
    }

    return 0;
}

VOID LogWriteExistingEntries(PLOG_CONTEXT LogCtx, BOOL IgnoreStopping)
{
    LIST_ENTRY LogEntries;
    PLIST_ENTRY ListEntry;
    PLOG_ENTRY LogEntry;

    if (LogCtx->Stopping && !IgnoreStopping)
        return;

    EnterCriticalSection(&LogCtx->Lock);
    if (LogCtx->Stopping && !IgnoreStopping) {
        LeaveCriticalSection(&LogCtx->Lock);
        return;
    }
    MoveList(&LogEntries, &LogCtx->ListHead);
    LeaveCriticalSection(&LogCtx->Lock);

    if (0 == LogFileLock(LogCtx)) {
        if (0 == LogFileSetPointerToEnd(LogCtx)) {
            for (ListEntry = LogEntries.Flink; ListEntry != &LogEntries; ListEntry = ListEntry->Flink) {
                LogEntry = CONTAINING_RECORD(ListEntry, LOG_ENTRY, ListEntry);
                LogFileWriteEntry(LogCtx, LogEntry);
            }
            FlushFileBuffers(LogCtx->hFile);
        }
        LogFileUnlock(LogCtx);
    }

    while (!IsListEmpty(&LogEntries)) {
        ListEntry = RemoveHeadList(&LogEntries);
        LogEntry = CONTAINING_RECORD(ListEntry, LOG_ENTRY, ListEntry);
        free(LogEntry);
    }
}

DWORD LogThreadRoutine(PLOG_CONTEXT LogCtx)
{
    while (!LogCtx->Stopping) {
        WaitForSingleObject(LogCtx->hEvent, INFINITE);
        if (!LogCtx->Stopping)
            LogWriteExistingEntries(LogCtx, FALSE);
    }

    return 0;
}

DWORD LogInit(PLOG_CONTEXT LogCtx, PWCHAR FilePath, ULONG Level)
{
    DWORD Err;

    memset(LogCtx, 0, sizeof(*LogCtx));
    LogCtx->Level = Level;
    InitializeListHead(&LogCtx->ListHead);
    _snwprintf(LogCtx->FilePath, RTL_NUMBER_OF(LogCtx->FilePath) - 1, L"%ws", FilePath);
    InitializeCriticalSection(&LogCtx->Lock);

    LogCtx->hEvent = CreateEvent(NULL, FALSE, FALSE, NULL);
    if (!LogCtx->hEvent) {
        Err = GetLastError();
        goto fail_delete_critical_section;
    }

    LogCtx->hFile = CreateFile(LogCtx->FilePath, FILE_GENERIC_WRITE, FILE_SHARE_READ|FILE_SHARE_WRITE,
                               NULL, OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
    if (LogCtx->hFile == INVALID_HANDLE_VALUE) {
        Err = GetLastError();
        goto fail_close_event;
    }

    LogCtx->hThread = CreateThread(NULL, 0, LogThreadRoutine, LogCtx, 0, NULL);
    if (!LogCtx->hThread) {
        Err = GetLastError();
        goto fail_close_file;
    }

    return 0;

fail_close_file:
    CloseHandle(LogCtx->hFile);
fail_close_event:
    CloseHandle(LogCtx->hEvent);
fail_delete_critical_section:
    DeleteCriticalSection(&LogCtx->Lock);
    return Err;
}

VOID LogRelease(PLOG_CONTEXT LogCtx)
{
    LogCtx->Stopping = 1;
    EnterCriticalSection(&LogCtx->Lock);
    LeaveCriticalSection(&LogCtx->Lock);

    WaitForSingleObject(LogCtx->hThread, INFINITE);

    LogWriteExistingEntries(LogCtx, TRUE);

    DeleteCriticalSection(&LogCtx->Lock);
    CloseHandle(LogCtx->hEvent);
    CloseHandle(LogCtx->hFile);
    CloseHandle(LogCtx->hThread);
}

BOOLEAN LogEntryEnqueue(PLOG_CONTEXT LogCtx, PLOG_ENTRY LogEntry)
{
    BOOLEAN Inserted;
    if (LogCtx->Stopping)
        return FALSE;

    EnterCriticalSection(&LogCtx->Lock);
    if (!LogCtx->Stopping) {
        InsertTailList(&LogCtx->ListHead, &LogEntry->ListEntry);
        Inserted = TRUE;
    } else
        Inserted = FALSE;
    LeaveCriticalSection(&LogCtx->Lock);
    if (Inserted)
        SetEvent(LogCtx->hEvent);

    return Inserted;
}

VOID Log(PLOG_CONTEXT LogCtx, ULONG Level, PCHAR File, ULONG Line, PCHAR Func, PCHAR Fmt, va_list Args)
{
    PLOG_ENTRY LogEntry;
    PCHAR BufPos;
    ULONG BufSize, BufLeft;
    SYSTEMTIME Time;

    if (Level < LogCtx->Level)
        return;

    LogEntry = malloc(sizeof(*LogEntry));
    if (!LogEntry)
        return;

    LogEntry->BufUsed = 0;
    BufPos = LogEntry->Buf;
    BufSize = sizeof(LogEntry->Buf);
    BufLeft = BufSize;

    switch (Level) {
    case LOG_INF:
        WriteMsg(&BufPos, &BufLeft, "INF");
        break;
    case LOG_ERR:
        WriteMsg(&BufPos, &BufLeft, "ERR");
        break;
    case LOG_DBG:
        WriteMsg(&BufPos, &BufLeft, "DBG");
        break;
    case LOG_WRN:
        WriteMsg(&BufPos, &BufLeft, "WRN");
        break;
    default:
        WriteMsg(&BufPos, &BufLeft, "UNK");
        break;
    }

    GetSystemTime(&Time);
    WriteMsg(&BufPos, &BufLeft," %02d:%02d:%02d.%03d ",
             Time.wHour, Time.wMinute,
             Time.wSecond, Time.wMilliseconds);

    WriteMsg(&BufPos, &BufLeft,"p%d t%d", GetCurrentProcessId(), GetCurrentThreadId());
    WriteMsg(&BufPos, &BufLeft," %s():%s:%d: ", Func, TruncatePath(File), Line);

    WriteMsg2(&BufPos, &BufLeft, Fmt, Args);
    if (WriteMsg(&BufPos, &BufLeft, "\r\n") <= 0) {
        LogEntry->Buf[BufSize-2] = '\r';
        LogEntry->Buf[BufSize-1] = '\n';
        LogEntry->BufUsed = BufSize;
    } else
        LogEntry->BufUsed = BufSize - BufLeft;

    if (!LogEntryEnqueue(LogCtx, LogEntry))
        free(LogEntry);
}

DWORD GlobalLogInit(PWCHAR FilePath, ULONG Level)
{
    return LogInit(&g_LogCtx, FilePath, Level);
}

VOID GlobalLogRelease()
{
    LogRelease(&g_LogCtx);
}

VOID GlobalLog(ULONG Level, PCHAR File, ULONG Line, PCHAR Func, PCHAR Fmt, ...)
{
    va_list Args;

    va_start(Args, Fmt);
    Log(&g_LogCtx, Level, File, Line, Func, Fmt, Args);
    va_end(Args);
}