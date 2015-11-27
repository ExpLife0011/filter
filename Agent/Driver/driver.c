#include "inc\driver.h"
#include "inc\klog.h"
#include "inc\mtags.h"
#include "inc\ntapiex.h"
#include "inc\fastio.h"
#include "inc\unload_protection.h"
#include "inc\helpers.h"
#include "h\ioctl.h"

FBDRIVER g_FbDriver;

PFBDRIVER GetFbDriver(VOID) {
    return &g_FbDriver;
}

NTSTATUS FbDriverInit(PFBDRIVER FbDriver)
{
    UnloadProtectionInit(&FbDriver->UnloadProtection);
    return WorkerStart(&FbDriver->MainWorker);
}

VOID FbDriverDeinit(PFBDRIVER FbDriver)
{
    WorkerStop(&FbDriver->MainWorker);
    if (FbDriver->NtfsDriver)
        ObDereferenceObject(FbDriver->NtfsDriver);
}

FAST_IO_DISPATCH g_FbFastIoDispatch = {
    .SizeOfFastIoDispatch = sizeof(FAST_IO_DISPATCH),
    .FastIoCheckIfPossible = FbFastIoCheckIfPossible,
    .FastIoRead = FbFastIoRead,
    .FastIoWrite = FbFastIoWrite,    
    .FastIoQueryBasicInfo = FbFastIoQueryBasicInfo,
    .FastIoQueryStandardInfo = FbFastIoQueryStandardInfo,
    .FastIoLock = FbFastIoLock,
    .FastIoUnlockSingle = FbFastIoUnlockSingle,
    .FastIoUnlockAll  = FbFastIoUnlockAll,
    .FastIoUnlockAllByKey = FbFastIoUnlockAllByKey,
    .FastIoDeviceControl = FbFastIoDeviceControl,
    .FastIoDetachDevice = FbFastIoDetachDevice,
    .FastIoQueryNetworkOpenInfo = FbFastIoQueryNetworkOpenInfo,
    .FastIoQueryOpen  = FbFastIoQueryOpen,
    .MdlRead = FbFastIoMdlRead,
    .MdlReadComplete = FbFastIoMdlReadComplete,
    .PrepareMdlWrite = FbFastIoPrepareMdlWrite,  
    .MdlWriteComplete = FbFastIoMdlWriteComplete,
    .FastIoReadCompressed = FbFastIoReadCompressed,
    .FastIoWriteCompressed = FbFastIoWriteCompressed,    
    .MdlReadCompleteCompressed = FbFastIoMdlReadCompleteCompressed,
    .MdlWriteCompleteCompressed = FbFastIoMdlWriteCompleteCompressed,
    .AcquireForCcFlush = NULL,
    .AcquireFileForNtCreateSection = NULL,
    .AcquireForModWrite = NULL,
    .ReleaseForCcFlush = NULL,    
    .ReleaseFileForNtCreateSection = NULL,
    .ReleaseForModWrite = NULL,
};

NTSTATUS 
    CompleteIrp( PIRP Irp, NTSTATUS status, ULONG info)
{
    Irp->IoStatus.Status = status;
    Irp->IoStatus.Information = info;
    IoCompleteRequest(Irp,IO_NO_INCREMENT);
    return status;
}

POBJECT_TYPE GetIoDriverObjectType(VOID)
{
    return ObGetObjectType(GetFbDriver()->Driver);
}

NTSTATUS GetNtfsDriverObject(PDRIVER_OBJECT *pDriver)
{
    UNICODE_STRING ObjName = RTL_CONSTANT_STRING(L"\\FileSystem\\Ntfs");

    KLInf("IoDriverObjectType %p", GetIoDriverObjectType());
    return GetObjectByName(&ObjName, GetIoDriverObjectType(), pDriver);
}

NTSTATUS GetDriverDevicesList(PDRIVER_OBJECT DriverObject, PDEVICE_OBJECT **pDeviceObjectList, ULONG *pNrDeviceObjects)
{
    ULONG NrDeviceObjects, DeviceObjectListSize;
    PDEVICE_OBJECT *DeviceObjectList;
    NTSTATUS Status;

    while (TRUE) {
        DeviceObjectList = NULL;
        NrDeviceObjects = 0;
        Status = IoEnumerateDeviceObjectList(DriverObject, DeviceObjectList, 0, &NrDeviceObjects);
        if (NrDeviceObjects == 0)
            break;

        DeviceObjectListSize = 2*NrDeviceObjects*sizeof(PDEVICE_OBJECT);
        DeviceObjectList = NpAlloc(2*NrDeviceObjects*sizeof(PDEVICE_OBJECT), MTAG_DRV);
        if (!DeviceObjectList) {
            KLErr("No memory");
            continue;
        }

        Status = IoEnumerateDeviceObjectList(DriverObject, DeviceObjectList, DeviceObjectListSize, &NrDeviceObjects);
        if (!NT_SUCCESS(Status)) {
            KLErr("IoEnumerateDeviceObjectList Status 0x%x", Status);
            NpFree(DeviceObjectList, MTAG_DRV);
            continue;
        }
        break;
    }

    *pDeviceObjectList = DeviceObjectList;
    *pNrDeviceObjects = NrDeviceObjects;
    return STATUS_SUCCESS;
}

VOID FbDevExtInit(PFBDEV_EXT DevExt, PDEVICE_OBJECT DeviceObject)
{
    RtlZeroMemory(DevExt, sizeof(*DevExt));
    DevExt->Magic = FBDEV_EXT_MAGIC;
    DevExt->DeviceObject = DeviceObject;
    KeInitializeEvent(&DevExt->ReleasedEvent, NotificationEvent, FALSE);
    DevExt->RefCount = 1;
}

NTSTATUS FbFltAttachDevice(PDEVICE_OBJECT DeviceObject)
{
    NTSTATUS Status;
    PFBDRIVER FbDriver = GetFbDriver();
    PDEVICE_OBJECT FltDevice;
    PFBDEV_EXT DevExt;
    ULONG i;

    Status = IoCreateDevice(FbDriver->Driver, sizeof(FBDEV_EXT), NULL, DeviceObject->DeviceType, 0, FALSE, &FltDevice);
    if (!NT_SUCCESS(Status)) {
        KLErr("Can't create filter device Status 0x%x", Status);
        return Status;
    }
    DevExt = (PFBDEV_EXT)FltDevice->DeviceExtension;
    FbDevExtInit(DevExt, FltDevice);
    for (i = 0; i < 10; i++) {
        Status = IoAttachDeviceToDeviceStackSafe(FltDevice, DeviceObject, &DevExt->AttachedToDevice);
        if (!NT_SUCCESS(Status)) {
            KLErr("Can't attach to device %p Status 0x%x", DeviceObject, Status);
            ThreadSleepMs(15);
        } else {
            KLInf("Attached FltDevice %p AttachedTo %p Target %p", FltDevice, DevExt->AttachedToDevice, DeviceObject);
            break;
        }
    }

    if (!NT_SUCCESS(Status)) {
        KLErr("Can't attach to device %p Status 0x%x", DeviceObject, Status);
        IoDeleteDevice(FltDevice);
        return Status;
    }
    FbDevExtReference(DevExt);

    if (FlagOn(DevExt->AttachedToDevice->Flags, DO_BUFFERED_IO))
        SetFlag(FltDevice->Flags, DO_BUFFERED_IO);
    else
        ClearFlag(FltDevice->Flags, DO_BUFFERED_IO);

    if (FlagOn(DevExt->AttachedToDevice->Flags, DO_DIRECT_IO))
        SetFlag(FltDevice->Flags, DO_DIRECT_IO);
    else
        ClearFlag(FltDevice->Flags, DO_DIRECT_IO);

    if (FlagOn(DevExt->AttachedToDevice->Characteristics, FILE_DEVICE_SECURE_OPEN))
        SetFlag(FltDevice->Characteristics, FILE_DEVICE_SECURE_OPEN);
    else
        ClearFlag(FltDevice->Characteristics, FILE_DEVICE_SECURE_OPEN);

    DevExt->TargetDevice = DeviceObject;
    ObReferenceObject(DevExt->TargetDevice);
    ObReferenceObject(DevExt->AttachedToDevice);

    ClearFlag(FltDevice->Flags, DO_DEVICE_INITIALIZING);

    return Status;
}

VOID FbFltDetachDevice(PDEVICE_OBJECT SourceDevice, PDEVICE_OBJECT TargetDevice)
{
    PFBDEV_EXT DevExt = FbDevExtByDevice(SourceDevice);

    KLInf("Going to detach Device %p AttachedToDevice %p TargetDevice %p",
          SourceDevice, DevExt->AttachedToDevice, TargetDevice);

    if (TargetDevice) {
        if (TargetDevice != DevExt->AttachedToDevice)
            __debugbreak();
        IoDetachDevice(TargetDevice);
    } else
        IoDetachDevice(DevExt->AttachedToDevice);
    FbDevExtDereference(DevExt);
}

NTSTATUS FbDriverFltStartWork(PFBDRIVER FbDriver)
{
    ULONG NrDeviceObjects;
    PDEVICE_OBJECT *DeviceObjectList;
    ULONG i;
    NTSTATUS Status;
    PDEVICE_OBJECT Device;

    Status = GetNtfsDriverObject(&FbDriver->NtfsDriver);
    KLInf("GetNtfsDriverObject Status 0x%x NtfsDriver %p", Status, FbDriver->NtfsDriver);
    if (!NT_SUCCESS(Status)) {
        goto Out;
    }

    Status = GetDriverDevicesList(FbDriver->NtfsDriver, &DeviceObjectList, &NrDeviceObjects);
    if (!NT_SUCCESS(Status)) {
        KLErr("Can't get devices list for driver %p Status 0x%x", GetFbDriver()->NtfsDriver, Status);
        __debugbreak();
        goto FailGetDevList;
    }

    for (i = 0; i < NrDeviceObjects; i++) {
        Device = DeviceObjectList[i];
        KLInf("Ntfs device %p", Device);
        Status  = FbFltAttachDevice(Device);
        if (!NT_SUCCESS(Status)) {
            KLErr("FbDriverAttachDevice Status 0x%x", Status);
        }
        ObDereferenceObject(Device);
    }
    NpFree(DeviceObjectList, MTAG_DRV);
    return STATUS_SUCCESS;
FailGetDevList:
    ObDereferenceObject(FbDriver->NtfsDriver);
    FbDriver->NtfsDriver = NULL;
Out:
    return Status;
}

NTSTATUS FbDriverFltStart(PFBDRIVER FbDriver)
{
    return WorkerExec(&FbDriver->MainWorker, FbDriverFltStartWork, FbDriver, TRUE);
}

NTSTATUS FbDriverFltStopWork(PFBDRIVER FbDriver)
{
    if (FbDriver->NtfsDriver) {
        ObDereferenceObject(FbDriver->NtfsDriver);
        FbDriver->NtfsDriver = NULL;
    }
    return STATUS_SUCCESS;
}

NTSTATUS FbDriverFltStop(PFBDRIVER FbDriver)
{
    return WorkerExec(&FbDriver->MainWorker, FbDriverFltStopWork, FbDriver, TRUE);
}

VOID FbFtlDevExtDumpStats(PFBDEV_EXT DevExt)
{
    int j;

    KLInf("Device %p DevExt %p IrpCount %d IrpCompletionCount %d FastIoCount %d FastIoSuccessCount %d",
          DevExt->DeviceObject, DevExt, DevExt->IrpCount, DevExt->IrpCompletionCount,
          DevExt->FastIoCount, DevExt->FastIoSuccessCount);

    for (j = 0; j < ARRAY_SIZE(DevExt->IrpMjCount); j++) {
        if (DevExt->IrpMjCount[j] != 0) {
            KLInf("Device %p IrpMjCount[0x%x]=%d", DevExt->DeviceObject, j, DevExt->IrpMjCount[j]);
        }
    }
    
    for (j = 0; j < ARRAY_SIZE(DevExt->FastIoCountByIndex); j++) {
        if (DevExt->FastIoCountByIndex[j] != 0) {
            KLInf("Device %p FastIoCountByIndex[0x%x]=%d", DevExt->DeviceObject, j, DevExt->FastIoCountByIndex[j]);
        }
    }
    
    for (j = 0; j < ARRAY_SIZE(DevExt->FastIoSuccessCountByIndex); j++) {
        if (DevExt->FastIoSuccessCountByIndex[j] != 0) {
            KLInf("Device %p FastIoSuccessCountByIndex[0x%x]=%d", DevExt->DeviceObject, j, DevExt->FastIoSuccessCountByIndex[j]);
        }
    }
}

VOID FbDriverDeviceDelete(PDEVICE_OBJECT Device)
{
    PFBDEV_EXT DevExt;

    DevExt = FbDevExtByDevice(Device);

    KLInf("Going to delete Device %p DevExt %p", Device, DevExt);
    FbDevExtDereference(DevExt);
    FbDevExtWaitRelease(DevExt);
    FbFtlDevExtDumpStats(DevExt);

    if (DevExt->SymLinkCreated) {
        KLInf("Deleting Device %p symlink %wZ", Device, &DevExt->SymLinkName);
        IoDeleteSymbolicLink(&DevExt->SymLinkName);
    }

    if (DevExt->AttachedToDevice) {
        ObDereferenceObject(DevExt->TargetDevice);
        ObDereferenceObject(DevExt->AttachedToDevice);
    }
    IoDeleteDevice(Device);
}

VOID DriverUnloadRoutine(IN PDRIVER_OBJECT DriverObject)
{
    ULONG NrDeviceObjects;
    PDEVICE_OBJECT *DeviceObjectList, Device;
    NTSTATUS Status;
    ULONG i;
    PFBDEV_EXT DevExt;
    PFBDRIVER FbDriver = GetFbDriver();

    KLInf("UnloadRoutine driver %p", DriverObject);

    Status = GetDriverDevicesList(DriverObject, &DeviceObjectList, &NrDeviceObjects);
    if (!NT_SUCCESS(Status)) {
        KLErr("Can't get devices list for driver %p Status 0x%x", DriverObject, Status);
        __debugbreak();
        return;
    }

    /* Detach devices */
    for (i = 0; i < NrDeviceObjects; i++) {
        Device = DeviceObjectList[i];
        DevExt = FbDevExtByDevice(Device);
        if (DevExt->AttachedToDevice != NULL)
            FbFltDetachDevice(Device, NULL);
    }

    /* Wait for all device I/O calm */
    UnloadProtectionWait(&FbDriver->UnloadProtection);

    /* Delete devices */
    for (i = 0; i < NrDeviceObjects; i++) {
        Device = DeviceObjectList[i];
        FbDriverDeviceDelete(Device);
        ObDereferenceObject(Device);
    }

    NpFree(DeviceObjectList, MTAG_DRV);
    FbDriverDeinit(FbDriver);

    KLInf("UnloadRoutine completed");
    KLogDeinit();
}

NTSTATUS DevCtlCtlHandler( IN PDEVICE_OBJECT Device, IN PIRP Irp )
{
    NTSTATUS Status = STATUS_SUCCESS;
    PFBDRIVER FbDriver = GetFbDriver();
    PIO_STACK_LOCATION IrpStack = IoGetCurrentIrpStackLocation(Irp);
    ULONG ControlCode = IrpStack->Parameters.DeviceIoControl.IoControlCode;
    ULONG method = ControlCode & 0x3;
    ULONG ResultLength = 0;
    ULONG InputLength = IrpStack->Parameters.DeviceIoControl.InputBufferLength;
    ULONG OutputLength = IrpStack->Parameters.DeviceIoControl.OutputBufferLength;
    PVOID Buffer = Irp->AssociatedIrp.SystemBuffer;

    KLInf("Device %p, Ctl 0x%x", Device, ControlCode);
    if (OutputLength < InputLength) {
        KLErr("Invalid OutputLen 0x%x vs InputLen 0x%x", OutputLength, InputLength);
        Status = STATUS_INVALID_PARAMETER;
        goto Done;
    }

    switch (ControlCode) {
    case IOCTL_FBACKUP_FLT_START:  
        Status = FbDriverFltStart(FbDriver);
        break;
    case IOCTL_FBACKUP_FLT_STOP:
        Status = FbDriverFltStop(FbDriver);
        break;
    case IOCTL_FBACKUP_TEST:
        Status = STATUS_NOT_SUPPORTED;
        break;
    case IOCTL_FBACKUP_ECHO:
        DbgPrint("Echo\n");
        Status = STATUS_SUCCESS;
        break;
    case IOCTL_FBACKUP_BUGCHECK:
        KeBugCheckEx(0x3B, 0, 0, 0, 0);
        break;
    default:
        Status = STATUS_INVALID_DEVICE_REQUEST;
        break;
    }
Done:
    KLInf("Device %p Ctl 0x%x Bytes 0x%x, Status 0x%x", Device, ControlCode, ResultLength, Status);
    Status = CompleteIrp(Irp, Status, ResultLength);
    return Status;
}

NTSTATUS
DevCtlIrpHandler(IN PDEVICE_OBJECT DeviceObject, IN PIRP Irp)
{
    NTSTATUS Status;
    PIO_STACK_LOCATION CurrentIrpStack = IoGetCurrentIrpStackLocation(Irp);

    KLInf("DevObj %p MJ 0x%x MN 0x%x",
        DeviceObject, CurrentIrpStack->MajorFunction, CurrentIrpStack->MinorFunction);

    switch (CurrentIrpStack->MajorFunction) {
    case IRP_MJ_DEVICE_CONTROL:
        Status = DevCtlCtlHandler(DeviceObject, Irp);
        break;
    case IRP_MJ_CREATE:
        Status = CompleteIrp(Irp, STATUS_SUCCESS, 0);
        break;
    case IRP_MJ_CLOSE:
        Status = CompleteIrp(Irp, STATUS_SUCCESS, 0);
        break;
    case IRP_MJ_CLEANUP:
        Status = CompleteIrp(Irp, STATUS_SUCCESS, 0);
        break;
    default:
        KLErr("Unsupported MJ 0x%x", CurrentIrpStack->MajorFunction);
        Status = CompleteIrp(Irp, STATUS_NOT_SUPPORTED, 0);
        break;
    }

    return Status;
}

NTSTATUS
    FbFltIrpCompletionRoutine(PDEVICE_OBJECT DeviceObject, PIRP Irp, PVOID Context)
{
    PFBDEV_IRP_CONTEXT IrpContext = (PFBDEV_IRP_CONTEXT)Context;
    PFBDEV_EXT DevExt = IrpContext->DevExt;

    if (IrpContext->Magic != FBDEV_IRP_CONTEXT_MAGIC)
        __debugbreak();
    if (IrpContext->Irp != Irp)
        __debugbreak();
    if (DevExt->Magic != FBDEV_EXT_MAGIC)
        __debugbreak();

    InterlockedIncrement(&DevExt->IrpCompletionCount);
    if (Irp->PendingReturned)
        IoMarkIrpPending(Irp);

    FbDevExtDereference(DevExt);
    NpFree(IrpContext, MTAG_DRV);

    return STATUS_SUCCESS;
}

NTSTATUS
    FbFltIrpPassThrough(PFBDEV_EXT DevExt, PIRP Irp)
{
    PFBDEV_IRP_CONTEXT IrpContext;

    IrpContext = NpAlloc(sizeof(*IrpContext), MTAG_DRV);
    if (!IrpContext) {
        __debugbreak();
        return CompleteIrp(Irp, STATUS_INSUFFICIENT_RESOURCES, 0);
    }

    IrpContext->DevExt = DevExt;
    IrpContext->Irp = Irp;
    IrpContext->Magic = FBDEV_IRP_CONTEXT_MAGIC;

    FbDevExtReference(DevExt);
    IoCopyCurrentIrpStackLocationToNext(Irp);
    IoSetCompletionRoutine(Irp, FbFltIrpCompletionRoutine, IrpContext, TRUE, TRUE, TRUE);
    return IoCallDriver(DevExt->AttachedToDevice, Irp);
}

NTSTATUS FbFltIrpWrite(PFBDEV_EXT DevExt, PIRP Irp, PIO_STACK_LOCATION IrpSp, PFILE_OBJECT FileObject, BOOLEAN *pbPassThrough)
{
    NTSTATUS Status;

    *pbPassThrough = TRUE;

    KLInf("Write: File %p %wZ MN 0x%x Offset 0x%llx Len 0x%x", FileObject, &FileObject->FileName, IrpSp->MinorFunction,
        IrpSp->Parameters.Write.ByteOffset.QuadPart, IrpSp->Parameters.Write.Length);
    Status = STATUS_SUCCESS;
    return Status;
}

NTSTATUS FbFltIrpFlushBuffers(PFBDEV_EXT DevExt, PIRP Irp, PIO_STACK_LOCATION IrpSp, PFILE_OBJECT FileObject, BOOLEAN *pbPassThrough)
{
    NTSTATUS Status;

    *pbPassThrough = TRUE;

    KLInf("FlushBuffers: File %p %wZ MN 0x%x", FileObject, &FileObject->FileName, IrpSp->MinorFunction);
    Status = STATUS_SUCCESS;
    return Status;
}

NTSTATUS FbFltIrpDefault(PFBDEV_EXT DevExt, PIRP Irp, PIO_STACK_LOCATION IrpSp, PFILE_OBJECT FileObject, BOOLEAN *pbPassThrough)
{
    NTSTATUS Status;

    *pbPassThrough = TRUE;
    Status = STATUS_SUCCESS;
    return Status;
}

NTSTATUS
    FbFltIrpHandler(PFBDEV_EXT DevExt, PIRP Irp)
{
    PIO_STACK_LOCATION CurrentIrpStack = IoGetCurrentIrpStackLocation(Irp);
    PFILE_OBJECT FileObject = CurrentIrpStack->FileObject;
    NTSTATUS Status;
    BOOLEAN bPassThrough;

    InterlockedIncrement(&DevExt->IrpCount);
    InterlockedIncrement(&DevExt->IrpMjCount[CurrentIrpStack->MajorFunction]);

    switch (CurrentIrpStack->MajorFunction) {
    case IRP_MJ_WRITE:
        Status = FbFltIrpWrite(DevExt, Irp, CurrentIrpStack, FileObject, &bPassThrough);
        break;
    case IRP_MJ_FLUSH_BUFFERS:
        Status = FbFltIrpFlushBuffers(DevExt, Irp, CurrentIrpStack, FileObject, &bPassThrough);
        break;
    default:
        Status = FbFltIrpDefault(DevExt, Irp, CurrentIrpStack, FileObject, &bPassThrough);
        break;
    }

    if (bPassThrough)
        return FbFltIrpPassThrough(DevExt, Irp);

    return Status;
}

NTSTATUS
DriverIrpHandler(IN PDEVICE_OBJECT DeviceObject, IN PIRP Irp)
{
    PFBDEV_EXT DevExt;
    PFBDRIVER FbDriver = GetFbDriver();
    NTSTATUS Status;

    UnloadProtectionAcquire(&FbDriver->UnloadProtection);

    DevExt = FbDevExtByDevice(DeviceObject);
    if (DeviceObject == GetFbDriver()->CtlDevice) {
        InterlockedIncrement(&DevExt->IrpCount);
        Status = DevCtlIrpHandler(DeviceObject, Irp);
        goto Ret;
    }

    if (DevExt->AttachedToDevice) {
        KLDbg("FltDevice %p Device %p Irp %p", DeviceObject, DevExt->AttachedToDevice, Irp);
        Status = FbFltIrpHandler(DevExt, Irp);
        goto Ret;
    }

    KLErr("Unhandled Device %p Irp %p\n", DeviceObject, Irp);
    Status = CompleteIrp(Irp, STATUS_NOT_SUPPORTED, 0);

Ret:
    UnloadProtectionRelease(&FbDriver->UnloadProtection);
    return Status;
}

NTSTATUS DriverEntry(IN PDRIVER_OBJECT DriverObject,
                     IN PUNICODE_STRING RegistryPath)
{
    NTSTATUS Status;
    PDEVICE_OBJECT  DeviceObject = NULL;
    UNICODE_STRING  DeviceName;
    ULONG i;
    PFBDRIVER FbDriver = GetFbDriver();

    DPRINT("DriverEntry Driver %p, RegPath %wZ\n", DriverObject, RegistryPath);
    Status = KLogInit();
    if (!NT_SUCCESS(Status)) {
        DPRINT("DriverLibInit failure Status 0x%x\n", Status);
        return Status;
    }

    Status = FbDriverInit(FbDriver);
    if (!NT_SUCCESS(Status)) {
        KLErr("FbDriverInit failure Status 0x%x", Status);
        goto FailFbDriverInit;
    }

    DriverObject->DriverUnload = DriverUnloadRoutine;
    for (i = 0; i <= IRP_MJ_MAXIMUM_FUNCTION; i++) {
        DriverObject->MajorFunction[i] = DriverIrpHandler;
    }
    DriverObject->FastIoDispatch = &g_FbFastIoDispatch;
    RtlInitUnicodeString( &DeviceName, FBACKUP_DRV_NT_DEVICE_NAME_W);

    KLInf("Driver %p, RegPath %wZ", DriverObject, RegistryPath);

    Status = IoCreateDevice(DriverObject,
        sizeof(FBDEV_EXT),
        &DeviceName,
        FILE_DEVICE_UNKNOWN,
        0,
        FALSE, 
        &DeviceObject);

    if(!NT_SUCCESS(Status)) {
        KLErr("Can't create ctl device Status 0x%x", Status);
        goto FailCreateDevice;
    }

    PFBDEV_EXT DevExt = (PFBDEV_EXT)DeviceObject->DeviceExtension;
    FbDevExtInit(DevExt, DeviceObject);

    KLInf("Device %p, DevExt %p", DeviceObject, DevExt);

    RtlInitUnicodeString(&DevExt->SymLinkName, FBACKUP_DRV_DOS_DEVICE_NAME_W );

    Status = IoCreateSymbolicLink( &DevExt->SymLinkName, &DeviceName );
    if (!NT_SUCCESS(Status)) {
        KLErr("Can't create symlink Status 0x%x", Status);
        goto FailCreateSymLink;
    }

    DevExt->SymLinkCreated = TRUE;
    FbDriver->CtlDevice = DeviceObject;
    FbDriver->Driver = DriverObject;
    KLInf("DriverEntry Status 0x%x", Status);

    return Status;
FailCreateSymLink:
    IoDeleteDevice(DeviceObject);
FailCreateDevice:
    FbDriverDeinit(FbDriver);
FailFbDriverInit:
    KLInf("DriverEntry Status 0x%x", Status);
    KLogDeinit();
    return Status;
}
