#include "inc\worker.h"
#include "inc\helpers.h"
#include "inc\klog.h"
#include "inc\mtags.h"

VOID WorkerRoutine(PVOID Context)
{
    PWORKER Worker = (PWORKER)Context;
    PWORKER_TASK Task;
    PLIST_ENTRY ListEntry;
    LIST_ENTRY TaskList;
    KIRQL Irql;

    while (!Worker->Stopping) {
        KeWaitForSingleObject(&Worker->WakeupEvent,
                              Executive,
                              KernelMode,
                              FALSE,
                              NULL);
        InitializeListHead(&TaskList);
        KeAcquireSpinLock(&Worker->Lock, &Irql);
        while (!IsListEmpty(&Worker->TaskListHead)) {
            ListEntry = RemoveHeadList(&Worker->TaskListHead);
            InsertTailList(&TaskList, ListEntry);
        }
        KeReleaseSpinLock(&Worker->Lock, Irql);

        while (!IsListEmpty(&TaskList)) {
            ListEntry = RemoveHeadList(&TaskList);
            Task = CONTAINING_RECORD(ListEntry, WORKER_TASK, ListEntry);
            Task->Status = Task->Clb(Task->Context);
            if (Task->bWait) {
                KeMemoryBarrier();
                KeSetEvent(&Task->CompleteEvent, 0, FALSE);
            } else
                NpFree(Task, MTAG_WRK);
        }
    }

    InitializeListHead(&TaskList);
    KeAcquireSpinLock(&Worker->Lock, &Irql);
    while (!IsListEmpty(&Worker->TaskListHead)) {
        ListEntry = RemoveHeadList(&Worker->TaskListHead);
        InsertTailList(&TaskList, ListEntry);
    }
    KeReleaseSpinLock(&Worker->Lock, Irql);

    while (!IsListEmpty(&TaskList)) {
        ListEntry = RemoveHeadList(&TaskList);
        Task = CONTAINING_RECORD(ListEntry, WORKER_TASK, ListEntry);
        Task->Status = STATUS_CANCELLED;
        if (Task->bWait) {
            KeMemoryBarrier();
            KeSetEvent(&Task->CompleteEvent, 0, FALSE);
        } else
            NpFree(Task, MTAG_WRK);
    }
}

VOID WorkerStop(PWORKER Worker)
{
    Worker->Stopping = TRUE;
    KeMemoryBarrier();
    KeSetEvent(&Worker->WakeupEvent, 0, FALSE);

    ZwWaitForSingleObject(Worker->hThread, 
                          FALSE,
                          NULL);
    ZwClose(Worker->hThread);

    if (Worker->Thread)
        ObDereferenceObject(Worker->Thread);
}

NTSTATUS WorkerStart(PWORKER Worker)
{
    NTSTATUS Status;
    OBJECT_ATTRIBUTES ObjectAttributes;

    RtlZeroMemory(Worker, sizeof(*Worker));
    InitializeListHead(&Worker->TaskListHead);
    KeInitializeSpinLock(&Worker->Lock);
    KeInitializeEvent(&Worker->WakeupEvent, SynchronizationEvent, FALSE);

    InitializeObjectAttributes(
        &ObjectAttributes, NULL,
        OBJ_KERNEL_HANDLE,
        NULL,NULL
        );

    Status = PsCreateSystemThread(&Worker->hThread,
                                  THREAD_ALL_ACCESS,
                                  &ObjectAttributes,
                                  0L,
                                  NULL,
                                  WorkerRoutine,
                                  Worker);
    if (!NT_SUCCESS(Status)) {
        KLErr("Can't create thread Status 0x%x", Status);
        return Status;
    }

    Status = ObReferenceObjectByHandle(Worker->hThread,
                                       THREAD_ALL_ACCESS,
                                       *PsThreadType,
                                       KernelMode,
                                       &Worker->Thread,
                                       NULL);
    if (!NT_SUCCESS(Status)) {
        KLErr("Can't create thread Status 0x%x", Status);
        WorkerStop(Worker);
        return Status;
    }

    return Status;
}

NTSTATUS WorkerExec(PWORKER Worker, PWORKER_CLB Clb, PVOID Context, BOOLEAN bWait)
{
    PWORKER_TASK Task;
    KIRQL Irql;
    NTSTATUS Status;

    Task = NpAlloc(sizeof(*Task), MTAG_WRK);
    if (!Task)
        return STATUS_INSUFFICIENT_RESOURCES;

    RtlZeroMemory(Task, sizeof(*Task));
    KeInitializeEvent(&Task->CompleteEvent, NotificationEvent, FALSE);
    Task->bWait = bWait;
    Task->Clb = Clb;
    Task->Context = Context;

    KeAcquireSpinLock(&Worker->Lock, &Irql);
    InsertTailList(&Worker->TaskListHead, &Task->ListEntry);
    KeReleaseSpinLock(&Worker->Lock, Irql);
    KeSetEvent(&Worker->WakeupEvent, 0, FALSE);

    if (!bWait)
        return STATUS_SUCCESS;

    KeWaitForSingleObject(&Task->CompleteEvent,
                          Executive,
                          KernelMode,
                          FALSE,
                          NULL);
    Status = Task->Status;
    NpFree(Task, MTAG_WRK);

    return Status;
}