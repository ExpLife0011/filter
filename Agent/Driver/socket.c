#include "inc\socket.h"
#include "inc\klog.h"
#include "inc\mtags.h"

PSOCKET SocketAlloc(PSOCKET_FACTORY SocketFactory)
{
    PSOCKET Socket;
    
    Socket = NpAlloc(sizeof(*Socket), MTAG_SOCKET);
    if (!Socket)
        return NULL;

    RtlZeroMemory(Socket, sizeof(*Socket));
    KeInitializeSpinLock(&Socket->Lock);
    Socket->RefCount = 1;
    Socket->SocketFactory = SocketFactory;
    return Socket;
}

VOID SocketFree(PSOCKET Socket)
{
    NpFree(Socket, MTAG_SOCKET);
}

NTSTATUS SocketFactoryInit(PSOCKET_FACTORY SocketFactory)
{
    NTSTATUS Status;

    RtlZeroMemory(SocketFactory, sizeof(*SocketFactory));
    InitializeListHead(&SocketFactory->SocketListHead);
    KeInitializeSpinLock(&SocketFactory->Lock);

    SocketFactory->ClientDispatch.Version = MAKE_WSK_VERSION(1, 0);
    SocketFactory->ClientDispatch.Reserved = 0;
    SocketFactory->ClientDispatch.WskClientEvent = NULL;
    
    SocketFactory->ClientNpi.ClientContext = NULL;
    SocketFactory->ClientNpi.Dispatch = &SocketFactory->ClientDispatch;

    Status = WskRegister(&SocketFactory->ClientNpi, &SocketFactory->Registration);
    if (!NT_SUCCESS(Status)) {
        KLErr("WskRegister failed Status 0x%x", Status);
        return Status;
    }

    Status = WskCaptureProviderNPI(&SocketFactory->Registration, WSK_INFINITE_WAIT,
                                   &SocketFactory->ProviderNpi);
    if (!NT_SUCCESS(Status)) {
        WskDeregister(&SocketFactory->Registration);
        KLErr("WskCaptureProviderNPI failed Status 0x%x", Status);
        return Status;
    }

    return Status;
}

NTSTATUS SocketWskDisconnectIrpCompletionRoutine(PDEVICE_OBJECT Device, PIRP Irp, PVOID Context)
{
    PKEVENT CompletionEvent = (PKEVENT)Context;

    KeSetEvent(CompletionEvent, 2, FALSE);
    return STATUS_MORE_PROCESSING_REQUIRED;
}

PWSK_PROVIDER_CONNECTION_DISPATCH WskSocketDispatch(PWSK_SOCKET WskSocket)
{
    return (PWSK_PROVIDER_CONNECTION_DISPATCH)WskSocket->Dispatch;
}

NTSTATUS SocketWskDisconnect(PWSK_SOCKET WskSocket, ULONG Flags)
{
    KEVENT CompEvent;
    NTSTATUS Status;
    PIRP Irp;

    KeInitializeEvent(&CompEvent, SynchronizationEvent, FALSE);

    Irp = IoAllocateIrp(1, FALSE);
    if (Irp == NULL) {
        KLErr( "No memory");
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    IoSetCompletionRoutine(Irp,
        SocketWskDisconnectIrpCompletionRoutine,
        &CompEvent, TRUE, TRUE, TRUE);

    Status = WskSocketDispatch(WskSocket)->WskDisconnect(WskSocket, NULL, Flags, Irp);
    KLDbg("WskDisconnect Status 0x%x", Status);

    KeWaitForSingleObject(&CompEvent, Executive, KernelMode, FALSE, NULL);

    Status = Irp->IoStatus.Status;

    if (!NT_SUCCESS(Status))
        KLErr("Disconnect Status 0x%x", Status);

    IoFreeIrp(Irp);

    return Status;
}

NTSTATUS SocketWskCloseIrpCompletionRoutine(PDEVICE_OBJECT Device, PIRP Irp, PVOID Context)
{
    PKEVENT CompletionEvent = (PKEVENT)Context;

    KeSetEvent(CompletionEvent, 2, FALSE);
    return STATUS_MORE_PROCESSING_REQUIRED;
}

NTSTATUS SocketWskClose(PWSK_SOCKET WskSocket)
{
    KEVENT CompletionEvent;
    NTSTATUS Status;
    PIRP Irp;

    KeInitializeEvent(&CompletionEvent, SynchronizationEvent, FALSE);

    Irp = IoAllocateIrp(1, FALSE);
    if (Irp == NULL) {
        KLErr("No memory");
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    IoSetCompletionRoutine(Irp,
        SocketWskCloseIrpCompletionRoutine,
        &CompletionEvent, TRUE, TRUE, TRUE);

    Status = WskSocketDispatch(WskSocket)->WskCloseSocket(WskSocket, Irp);
    KLDbg("WskCloseSocket Status 0x%x", Status);

    KeWaitForSingleObject(&CompletionEvent, Executive, KernelMode, FALSE, NULL);
    Status = Irp->IoStatus.Status;

    if (!NT_SUCCESS(Status))
        KLErr("Close Status 0x%x", Status);

    IoFreeIrp(Irp);

    return Status;
}

VOID SocketShutdown(PSOCKET Socket)
{
    KIRQL Irql;
    PWSK_SOCKET WskSocket;

    KeAcquireSpinLock(&Socket->Lock, &Irql);
    WskSocket = Socket->WskSocket;
    Socket->WskSocket = NULL;
    KeReleaseSpinLock(&Socket->Lock, Irql);
    if (WskSocket) {
        SocketWskDisconnect(WskSocket, 0);
        SocketWskClose(WskSocket);
    }
}

VOID SocketDereference(PSOCKET Socket)
{
    if (Socket->RefCount <= 0)
        __debugbreak();

    if (0 == InterlockedDecrement(&Socket->RefCount)) {
        SocketShutdown(Socket);
        SocketFree(Socket);
    }
}

VOID SocketFactoryRelease(PSOCKET_FACTORY SocketFactory)
{
    LIST_ENTRY SocketList;
    PLIST_ENTRY ListEntry;
    PSOCKET Socket;
    KIRQL Irql;

    SocketFactory->Releasing = 1;
    InitializeListHead(&SocketList);
    KeAcquireSpinLock(&SocketFactory->Lock, &Irql);
    while (!IsListEmpty(&SocketFactory->SocketListHead)) {
        ListEntry = RemoveHeadList(&SocketFactory->SocketListHead);
        InsertTailList(&SocketList, ListEntry);
    }
    KeReleaseSpinLock(&SocketFactory->Lock, Irql);

    while (!IsListEmpty(&SocketList)) {
        ListEntry = RemoveHeadList(&SocketList);
        Socket = CONTAINING_RECORD(ListEntry, SOCKET, ListEntry);
        SocketShutdown(Socket);
        SocketDereference(Socket);
    }
    WskReleaseProviderNPI(&SocketFactory->Registration);
    WskDeregister(&SocketFactory->Registration);
}

NTSTATUS SocketWskResolveNameCompletionRoutine(PDEVICE_OBJECT Device, PIRP Irp, PVOID Context)
{
    PKEVENT CompletionEvent = (PKEVENT)Context;

    KeSetEvent(CompletionEvent, 2, FALSE);
    return STATUS_MORE_PROCESSING_REQUIRED;
}

NTSTATUS SocketFactoryResolveNameInternal(PSOCKET_FACTORY SocketFactory, PUNICODE_STRING NodeName,
                                          PUNICODE_STRING ServiceName, PADDRINFOEXW Hints,
                                          PSOCKADDR_IN ResolvedAddress)
{
    NTSTATUS Status;
    PIRP Irp;
    KEVENT CompletionEvent;
    PADDRINFOEXW Results = NULL, AddrInfo = NULL;

    KeInitializeEvent(&CompletionEvent, SynchronizationEvent, FALSE);

    Irp = IoAllocateIrp(1, FALSE);
    if (Irp == NULL) {
        KLErr("No memory");
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    IoSetCompletionRoutine(Irp, SocketWskResolveNameCompletionRoutine, &CompletionEvent, TRUE, TRUE, TRUE);

    SocketFactory->ProviderNpi.Dispatch->WskGetAddressInfo(
        SocketFactory->ProviderNpi.Client,
        NodeName,
        ServiceName,
        NS_ALL,
        NULL,
        Hints,
        &Results,
        NULL,
        NULL,
        Irp);

    KeWaitForSingleObject(&CompletionEvent, Executive,
        KernelMode, FALSE, NULL);

    Status = Irp->IoStatus.Status;

    IoFreeIrp(Irp);

    if (!NT_SUCCESS(Status)) {
        KLErr("ResolveName Status 0x%x", Status);
        return Status;
    }

    AddrInfo = Results;
    if (AddrInfo != NULL) {
        *ResolvedAddress = *((PSOCKADDR_IN)(AddrInfo->ai_addr));
    } else {
        Status = STATUS_UNSUCCESSFUL;
        KLErr("No addresses found");
    }

    SocketFactory->ProviderNpi.Dispatch->WskFreeAddressInfo(
        SocketFactory->ProviderNpi.Client,
        Results);

    return Status;
}

NTSTATUS SocketFactoryResolveName(PSOCKET_FACTORY SocketFactory, PWCHAR Ip, PWCHAR Port,
                                  PSOCKADDR_IN ResolvedAddress)
{
    UNICODE_STRING NodeName, ServiceName;
    SOCKADDR_IN Address;
    NTSTATUS Status;

    RtlInitUnicodeString(&NodeName, Ip);
    RtlInitUnicodeString(&ServiceName, Port);

    Status = SocketFactoryResolveNameInternal(SocketFactory, &NodeName, &ServiceName, NULL, &Address);
    if (!NT_SUCCESS(Status)) {
        KLErr("Can't resolve Ip %ws Port %ws Status 0x%x", Ip, Port, Status);
        return Status;
    }

    *ResolvedAddress = Address;
    return Status;
}

NTSTATUS SocketWskConnectIrpCompletionRoutine(PDEVICE_OBJECT Device, PIRP Irp, PVOID Context)
{
    PKEVENT CompletionEvent = (PKEVENT)Context;

    KeSetEvent(CompletionEvent, 2, FALSE);
    return STATUS_MORE_PROCESSING_REQUIRED;
}

NTSTATUS SocketConnectInternal(PSOCKET_FACTORY SocketFactory, USHORT SocketType, ULONG Protocol,
                               PSOCKADDR LocalAddr, PSOCKADDR RemoteAddr, PSOCKET *pSocket)
{
    PSOCKET Socket;
    PIRP Irp;
    KEVENT CompletionEvent;
    KIRQL Irql;
    NTSTATUS Status;

    Socket = SocketAlloc(SocketFactory);
    if (!Socket) {
        KLErr("No memory");
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    Irp = IoAllocateIrp(1, FALSE);
    if (Irp == NULL) {
        KLErr("No memory");
        Status = STATUS_INSUFFICIENT_RESOURCES;
        goto FailAllocIrp;
    }

    KeInitializeEvent(&CompletionEvent, NotificationEvent, FALSE);
    IoSetCompletionRoutine(Irp,
        SocketWskConnectIrpCompletionRoutine,
        &CompletionEvent, TRUE, TRUE, TRUE);

    Status = SocketFactory->ProviderNpi.Dispatch->WskSocketConnect(SocketFactory->ProviderNpi.Client,
        SocketType,
        Protocol,
        LocalAddr,
        RemoteAddr,
        0,
        NULL,
        NULL,
        NULL,
        NULL,
        NULL,
        Irp);
    KLDbg("WskSocketConnect Status 0x%x", Status);
    if (!NT_SUCCESS(Status)) {
        KLErr("WskSocketConnect Status 0x%x", Status);
        goto FailConnect;
    }

    KeWaitForSingleObject(&CompletionEvent, Executive, KernelMode, FALSE, NULL);

    Status = Irp->IoStatus.Status;
    if (!NT_SUCCESS(Status)) {
        KLErr("WskSocketConnect status 0x%x", Status);
        goto FailConnect;
    }

    Socket->WskSocket = (PWSK_SOCKET)Irp->IoStatus.Information;
    KeAcquireSpinLock(&SocketFactory->Lock, &Irql);
    if (SocketFactory->Releasing) {
        KLErr("Socket factory releasing");
        Status = STATUS_TOO_LATE;
    } else {
        SocketReference(Socket);
        InsertHeadList(&SocketFactory->SocketListHead, &Socket->ListEntry);
        Status = STATUS_SUCCESS;
    }
    KeReleaseSpinLock(&SocketFactory->Lock, Irql);
    if (!NT_SUCCESS(Status))
        goto FailSocketInsert;

    *pSocket = Socket;
    IoFreeIrp(Irp);
    return Status;

FailSocketInsert:
    SocketShutdown(Socket);
FailConnect:
    IoFreeIrp(Irp);
FailAllocIrp:
    SocketDereference(Socket);
    return Status;
}

NTSTATUS SocketConnect(PSOCKET_FACTORY SocketFactory, PWCHAR Ip, PWCHAR Port, PSOCKET *pSocket)
{
    NTSTATUS Status;
    SOCKADDR_IN LocalAddr, RemoteAddr;

    Status = SocketFactoryResolveName(SocketFactory, Ip, Port, &RemoteAddr);
    if (!NT_SUCCESS(Status)) {
        KLErr("Can't resolve Ip %ws Port %ws Status 0x%x", Ip, Port, Status);
        return Status;
    }

    IN4ADDR_SETANY(&LocalAddr);
    return SocketConnectInternal(SocketFactory, SOCK_STREAM, IPPROTO_TCP, (PSOCKADDR)&LocalAddr, (PSOCKADDR)&RemoteAddr, pSocket);
}

NTSTATUS SocketWskSendIrpCompletionRoutine(PDEVICE_OBJECT Device, PIRP Irp, PVOID Context)
{
    PKEVENT CompEvent = (PKEVENT)Context;

    KeSetEvent(CompEvent, 2, FALSE);
    return STATUS_MORE_PROCESSING_REQUIRED;
}

NTSTATUS SocketSendInternal(PSOCKET Socket, ULONG Flags, PVOID Buf, ULONG Size, PULONG pSent)
{
    WSK_BUF WskBuf;
    KEVENT CompEvent;
    PIRP Irp = NULL;
    PMDL Mdl = NULL;
    NTSTATUS Status;
    LARGE_INTEGER Timeout;

    *pSent = 0;
    KeInitializeEvent(&CompEvent, NotificationEvent, FALSE);

    Irp = IoAllocateIrp(1, FALSE);
    if (Irp == NULL) {
        KLErr("No memory");
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    IoSetCompletionRoutine(Irp,
        SocketWskSendIrpCompletionRoutine,
        &CompEvent, TRUE, TRUE, TRUE);

    Mdl = IoAllocateMdl(Buf, Size, FALSE, FALSE, NULL);
    if (Mdl == NULL) {
        KLErr("No memory");
        Status = STATUS_INSUFFICIENT_RESOURCES;
        goto Cleanup;
    }
    MmBuildMdlForNonPagedPool(Mdl);

    WskBuf.Offset = 0;
    WskBuf.Length = Size;
    WskBuf.Mdl = Mdl;

    KLDbg( "Send irp %p", Irp);

    Status = WskSocketDispatch(Socket->WskSocket)->WskSend(Socket->WskSocket, &WskBuf, Flags, Irp);
    KLDbg( "WskSend Status 0x%x", Status);

    /* Wait maximum 10 secs */
    Timeout.QuadPart = -MillisTo100Ns(10000);
    Status = KeWaitForSingleObject(&CompEvent, Executive, KernelMode, FALSE, &Timeout);
    if (Status == STATUS_TIMEOUT) {
        KLErr( "Wait timed out will cancel irp %p", Irp);
        if (!IoCancelIrp(Irp)) {
            KLErr("Cant cancel irp %p", Irp);
        }
        KLDbg("Start waiting for event signaled");
        KeWaitForSingleObject(&CompEvent, Executive, KernelMode, FALSE, NULL);
        KLDbg("Waited for event signaled");
    }

    Status = Irp->IoStatus.Status;

    if (!NT_SUCCESS(Status))
        KLErr("Send Status 0x%x", Status);

    if (NT_SUCCESS(Status)) {
        *pSent = (ULONG)Irp->IoStatus.Information;
    }
Cleanup:
    if (Irp != NULL)
        IoFreeIrp(Irp);
    if (Mdl != NULL)
        IoFreeMdl(Mdl);

    return Status;
}

NTSTATUS SocketSend(PSOCKET Socket, PVOID Buf, ULONG Size, PULONG pSent)
{
    ULONG BytesSent;
    ULONG Offset;
    NTSTATUS Status = STATUS_UNSUCCESSFUL;

    Offset = 0;
    while (Offset < Size) {
        Status = SocketSendInternal(Socket, WSK_FLAG_NODELAY, (PVOID)((ULONG_PTR)Buf + Offset), Size - Offset, &BytesSent);
        if (!NT_SUCCESS(Status)) {
            KLErr("Send failed with Status 0x%x", Status);
            break;
        }
        Offset += BytesSent;
    }

    *pSent = Offset;

    return Status;
}

NTSTATUS SocketWskReceiveIrpCompletionRoutine(PDEVICE_OBJECT Device, PIRP Irp, PVOID Context)
{
    PKEVENT CompEvent = (PKEVENT)Context;

    KeSetEvent(CompEvent, 2, FALSE);
    return STATUS_MORE_PROCESSING_REQUIRED;
}

NTSTATUS SocketReceiveInternal(PSOCKET Socket, ULONG Flags, PVOID Buf, ULONG Size, ULONG *pReceived)
{
    WSK_BUF WskBuf;
    KEVENT CompEvent;
    PIRP Irp = NULL;
    PMDL Mdl = NULL;
    NTSTATUS Status;
    LARGE_INTEGER Timeout;

    *pReceived = 0;
    KeInitializeEvent(&CompEvent, SynchronizationEvent, FALSE);

    Irp = IoAllocateIrp(1, FALSE);
    if (Irp == NULL) {
        KLErr( "No memory");
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    IoSetCompletionRoutine(Irp,
        SocketWskReceiveIrpCompletionRoutine,
        &CompEvent, TRUE, TRUE, TRUE);

    Mdl = IoAllocateMdl(Buf, Size, FALSE, FALSE, NULL);
    if (Mdl == NULL) {
        KLErr("No memory");
        Status = STATUS_INSUFFICIENT_RESOURCES;
        goto Cleanup;
    }
    MmBuildMdlForNonPagedPool(Mdl);

    WskBuf.Offset = 0;
    WskBuf.Length = Size;
    WskBuf.Mdl = Mdl;

    KLDbg("Receive irp %p", Irp);

    Status = WskSocketDispatch(Socket->WskSocket)->WskReceive(Socket->WskSocket, &WskBuf, Flags, Irp);
    KLDbg("WskReceive Status 0x%x", Status);

    /* Wait maximum 10 secs */
    Timeout.QuadPart = -MillisTo100Ns(10000);
    Status = KeWaitForSingleObject(&CompEvent, Executive, KernelMode, FALSE, &Timeout);
    if (Status == STATUS_TIMEOUT) {
        KLErr( "Wait timed out will cancel irp %p", Irp);
        if (!IoCancelIrp(Irp)) {
            KLErr("Cant cancel irp %p", Irp);
        }
        KLDbg("Start waiting for event signaled");
        KeWaitForSingleObject(&CompEvent, Executive, KernelMode, FALSE, NULL);
        KLDbg("Waited for event signaled");
    }

    Status = Irp->IoStatus.Status;

    if (!NT_SUCCESS(Status))
        KLErr( "Receive status 0x%x", Status);

    if (NT_SUCCESS(Status)) {
        *pReceived = (ULONG)Irp->IoStatus.Information;
    }

Cleanup:
    if (Irp != NULL)
        IoFreeIrp(Irp);
    if (Mdl != NULL)
        IoFreeMdl(Mdl);

    return Status;
}

NTSTATUS SocketReceive(PSOCKET Socket, PVOID Buf, ULONG Size, PULONG pReceived, PBOOLEAN pbDisconnected)
{
    ULONG BytesRcv;
    ULONG Offset;
    NTSTATUS Status = STATUS_UNSUCCESSFUL;

    Offset = 0;
    while (Offset < Size) {
        Status = SocketReceiveInternal(Socket, 0, (PVOID)((ULONG_PTR)Buf + Offset), Size - Offset, &BytesRcv);
        if (!NT_SUCCESS(Status)) {
            KLErr("Received Status 0x%x", Status);
            break;
        }

        if (BytesRcv == 0) {
            KLDbg("Received 0 bytes");
            *pbDisconnected = TRUE;
            Status = STATUS_SUCCESS;
            break;
        }
        Offset += BytesRcv;
    }

    *pReceived = Offset;

    return Status;
}

VOID SocketClose(PSOCKET Socket)
{
    PSOCKET_FACTORY SocketFactory = Socket->SocketFactory;
    KIRQL Irql;

    SocketShutdown(Socket);

    KeAcquireSpinLock(&SocketFactory->Lock, &Irql);
    RemoveEntryList(&Socket->ListEntry);
    KeReleaseSpinLock(&SocketFactory->Lock, Irql);

    SocketDereference(Socket);
    SocketDereference(Socket);
}
