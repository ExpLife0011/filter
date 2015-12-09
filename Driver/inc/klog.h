#ifndef __FBACKUP_KLOG_H__
#define __FBACKUP_KLOG_H__

#include "inc/base.h"
#include "inc/hallocator.h"

#define KLOG_PATH L"\\??\\C:\\Windows\\System32\\fbackup.log"
#define KLOG_MSG_SZ 488

typedef struct _KLOG_BUFFER {
    LIST_ENTRY  ListEntry;
    CHAR        Msg[KLOG_MSG_SZ];
    ULONG       Length;
} KLOG_BUFFER, *PKLOG_BUFFER;

C_ASSERT(sizeof(KLOG_BUFFER) == 512);

typedef struct _KLOG_CONTEXT {
    volatile BOOLEAN    Stopping;
    HANDLE              ThreadHandle;
    PVOID               Thread;
    HANDLE              FileHandle;
    LIST_ENTRY          FlushQueue;
    LIST_ENTRY          FreeList;
    KSPIN_LOCK          Lock;
    KEVENT              FlushEvent;
    KDPC                Dpc;
    HALLOCATOR          LogBufAllocator;
} KLOG_CONTEXT, *PKLOG_CONTEXT;

PKLOG_CONTEXT KLogCreate(PUNICODE_STRING FileName);
VOID KLogRelease(PKLOG_CONTEXT Log);

enum {
    KL_DBG,
    KL_INF,
    KL_WRN,
    KL_ERR
};

VOID KLogInternal(int level, PCHAR file, ULONG line, PCHAR func, const char *fmt, ...);

#define KLOG_ENABLED 1
#define KLOG_LEVEL KL_INF

#if (KLOG_ENABLED)
#define KLog(level, fmt, ...) do {                                                      \
    if ((level) >= KLOG_LEVEL) {                                                        \
        KLogInternal((level), __FILE__, __LINE__, __FUNCTION__, (fmt), ##__VA_ARGS__);  \
    }                                                                                   \
} while (0);
#else
#define KLog(level, fmt, ...)
#endif

#define KLErr(fmt, ...)   \
            KLog(KL_ERR, fmt, ##__VA_ARGS__)

#define KLDbg(fmt, ...)   \
            KLog(KL_DBG, fmt, ##__VA_ARGS__)

#define KLInf(fmt, ...)   \
            KLog(KL_INF, fmt, ##__VA_ARGS__)

#define KLWrn(fmt, ...)   \
            KLog(KL_WRN, fmt, ##__VA_ARGS__)

extern PKLOG_CONTEXT g_Log;

NTSTATUS KLogInit();
VOID KLogDeinit();

void GetLocalTimeFields(PTIME_FIELDS pTimeFields);

#define KLOG_DBG_PRINT
#endif