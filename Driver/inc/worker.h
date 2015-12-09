#ifndef __FBACKUP_WORKER_H__
#define __FBACKUP_WORKER_H__

#include "inc\base.h"

typedef NTSTATUS (*PWORKER_CLB)(PVOID Context);

typedef struct _WORKER_TASK {
    LIST_ENTRY  ListEntry;
    NTSTATUS    Status;
    KEVENT      CompleteEvent;
    PWORKER_CLB Clb;
    PVOID       Context;
    BOOLEAN     bWait;
} WORKER_TASK, *PWORKER_TASK;

typedef struct _WORKER {
    PETHREAD            Thread;
    HANDLE              hThread;
    LIST_ENTRY          TaskListHead;
    KSPIN_LOCK          Lock;
    KEVENT              WakeupEvent;
    volatile BOOLEAN    Stopping;
} WORKER, *PWORKER;

NTSTATUS WorkerStart(PWORKER Worker);
VOID WorkerStop(PWORKER Worker);

NTSTATUS WorkerExec(PWORKER Worker, PWORKER_CLB Clb, PVOID Context, BOOLEAN bWait);

#endif