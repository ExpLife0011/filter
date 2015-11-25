#include "inc/driver.h"
#include "inc/klog.h"
#include "inc/mtags.h"
#include "inc/ntapiex.h"
#include "inc/fastio.h"
#include "inc/unload_protection.h"
#include "inc/helpers.h"
#include "h/ioctl.h"

#define FBDEV_EXT_MAGIC 0xCBDACBDA

typedef struct _FBDEV_EXT
{
    ULONG Magic;
    PDEVICE_OBJECT DeviceObject;
    UNICODE_STRING SymLinkName;
    BOOLEAN SymLinkCreated;
    PDEVICE_OBJECT TargetDevice;
    PDEVICE_OBJECT AttachedToDevice;
    volatile LONG   IrpMjCount[IRP_MJ_MAXIMUM_FUNCTION+1];
    volatile LONG   IrpCount;
} FBDEV_EXT, *PFBDEV_EXT;

typedef struct _FBDRIVER {
    PDRIVER_OBJECT Driver;
    PDEVICE_OBJECT CtlDevice;
    PDRIVER_OBJECT NtfsDriver;
    UNLOAD_PROTECTION UnloadProtection;
    KGUARDED_MUTEX Lock;
} FBDRIVER, *PFBDRIVER;

FBDRIVER g_FbDriver;

FORCEINLINE PFBDRIVER GetFbDriver(VOID) {
    return &g_FbDriver;
}

VOID FbDriverInit(VOID)
{
    PFBDRIVER FbDriver = GetFbDriver();
    
    KeInitializeGuardedMutex(&FbDriver->Lock);
    UnloadProtectionInit(&FbDriver->UnloadProtection);
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
    .AcquireFileForNtCreateSection = FbFastIoAcquireFile,
    .ReleaseFileForNtCreateSection = FbFastIoReleaseFile,
    .FastIoDetachDevice = FbFastIoDetachDevice,
    .FastIoQueryNetworkOpenInfo = FbFastIoQueryNetworkOpenInfo,
    .AcquireForModWrite = FbFastIoAcquireForModWrite,
    .MdlRead = FbFastIoMdlRead,
    .MdlReadComplete = FbFastIoMdlReadComplete,
    .PrepareMdlWrite = FbFastIoPrepareMdlWrite,  
    .MdlWriteComplete = FbFastIoMdlWriteComplete,
    .FastIoReadCompressed = FbFastIoReadCompressed,
    .FastIoWriteCompressed = FbFastIoWriteCompressed,    
    .MdlReadCompleteCompressed = FbFastIoMdlReadCompleteCompressed,
    .MdlWriteCompleteCompressed = FbFastIoMdlWriteCompleteCompressed,
    .ReleaseForModWrite = FbFastIoReleaseForModWrite,
    .AcquireForCcFlush = FbFastIoAcquireForCcFlush,
    .ReleaseForCcFlush = FbFastIoReleaseForCcFlush,    
    .FastIoQueryOpen  = FbFastIoQueryOpen
};

NTSTATUS 
    CompleteIrp( PIRP Irp, NTSTATUS status, ULONG info)
{
    Irp->IoStatus.Status = status;
    Irp->IoStatus.Information = info;
    IoCompleteRequest(Irp,IO_NO_INCREMENT);
    return status;
}

NTSTATUS GetNtfsDriverObject(PDRIVER_OBJECT *pDriver)
{
    UNICODE_STRING ObjName = RTL_CONSTANT_STRING(L"\\FileSystem\\Ntfs");

    KLInf("IoDriverObjectType %p", *IoDriverObjectType);
    return GetObjectByName(&ObjName, *IoDriverObjectType, pDriver);
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

NTSTATUS FbDriverAttachDevice(PDEVICE_OBJECT DeviceObject)
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
    RtlZeroMemory(DevExt, sizeof(*DevExt));
    DevExt->DeviceObject = DeviceObject;
    DevExt->Magic = FBDEV_EXT_MAGIC;
    for (i = 0; i < 10; i++) {
        Status = IoAttachDeviceToDeviceStackSafe(FltDevice, DeviceObject, &DevExt->AttachedToDevice);
        if (!NT_SUCCESS(Status)) {
            LARGE_INTEGER WaitInterval;
            KLErr("Can't attach to device %p Status 0x%x", DeviceObject, Status);
            WaitInterval.QuadPart = -10*1000*1000*10; /* 10ms */
            KeDelayExecutionThread(KernelMode, FALSE, &WaitInterval);
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

    if (FlagOn(DevExt->AttachedToDevice->Flags, DO_BUFFERED_IO))
        SetFlag(FltDevice->Flags, DO_BUFFERED_IO);

    if (FlagOn(DevExt->AttachedToDevice->Flags, DO_DIRECT_IO))
        SetFlag(FltDevice->Flags, DO_BUFFERED_IO);

    if (FlagOn(DevExt->AttachedToDevice->Characteristics, FILE_DEVICE_SECURE_OPEN))
        SetFlag(FltDevice->Characteristics, FILE_DEVICE_SECURE_OPEN);

    DevExt->TargetDevice = DeviceObject;
    ObReferenceObject(DevExt->TargetDevice);
    ObReferenceObject(DevExt->AttachedToDevice);

    ClearFlag(FltDevice->Flags, DO_DEVICE_INITIALIZING);

    return Status;
}

NTSTATUS FbDriverStart(VOID)
{
    PFBDRIVER FbDriver = GetFbDriver();
    ULONG NrDeviceObjects;
    PDEVICE_OBJECT *DeviceObjectList;
    ULONG i;
    NTSTATUS Status;
    PDEVICE_OBJECT Device;

    KeAcquireGuardedMutex(&FbDriver->Lock);
    Status = GetNtfsDriverObject(&FbDriver->NtfsDriver);
    KLInf("GetNtfsDriverObject Status 0x%x NtfsDriver %p", Status, FbDriver->NtfsDriver);
    if (!NT_SUCCESS(Status)) {
        goto Out;
    }

    Status = GetDriverDevicesList(FbDriver->NtfsDriver, &DeviceObjectList, &NrDeviceObjects);
    if (!NT_SUCCESS(Status)) {
        KLErr("Can't get devices list for driver %p Status 0x%x", GetFbDriver()->NtfsDriver, Status);
        __debugbreak();
        goto Fail_get_dev_list;
    }

    for (i = 0; i < NrDeviceObjects; i++) {
        Device = DeviceObjectList[i];
        KLInf("Ntfs device %p", Device);
        Status  = FbDriverAttachDevice(Device);
        if (!NT_SUCCESS(Status)) {
            KLErr("FbDriverAttachDevice Status 0x%x", Status);
        }
        ObDereferenceObject(Device);
    }
    NpFree(DeviceObjectList, MTAG_DRV);
    KeReleaseGuardedMutex(&FbDriver->Lock);
    return STATUS_SUCCESS;
Fail_get_dev_list:
    ObDereferenceObject(FbDriver->NtfsDriver);
    FbDriver->NtfsDriver = NULL;
Out:
    KeReleaseGuardedMutex(&FbDriver->Lock);
    return Status;
}

VOID FbDriverStop(VOID)
{
    PFBDRIVER FbDriver = GetFbDriver();

    KeAcquireGuardedMutex(&FbDriver->Lock);
    if (FbDriver->NtfsDriver) {
        ObDereferenceObject(FbDriver->NtfsDriver);
        FbDriver->NtfsDriver = NULL;
    }
    KeReleaseGuardedMutex(&FbDriver->Lock);
}

NTSTATUS FbCtlInit(VOID)
{
    return FbDriverStart();
}

NTSTATUS FbCtlRelease(VOID)
{
    FbDriverStop();
    return STATUS_SUCCESS;
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
    FbDriverStop();

    Status = GetDriverDevicesList(DriverObject, &DeviceObjectList, &NrDeviceObjects);
    if (!NT_SUCCESS(Status)) {
        KLErr("Can't get devices list for driver %p Status 0x%x", DriverObject, Status);
        __debugbreak();
        return;
    }

    /* Detach devices */
    for (i = 0; i < NrDeviceObjects; i++) {
        Device = DeviceObjectList[i];
        DevExt = (PFBDEV_EXT)Device->DeviceExtension;
        if (DevExt->Magic != FBDEV_EXT_MAGIC)
            __debugbreak();
        
        if (DevExt->AttachedToDevice) {
            KLInf("Going to detach Device %p AttachedToDevice %p", Device, DevExt->AttachedToDevice);
            IoDetachDevice(DevExt->AttachedToDevice);
        }
    }

    /* Wait for all device I/O calm */
    UnloadProtectionWait(&FbDriver->UnloadProtection);

    /* Delete devices */
    for (i = 0; i < NrDeviceObjects; i++) {
        Device = DeviceObjectList[i];
        DevExt = (PFBDEV_EXT)Device->DeviceExtension;
        if (DevExt->Magic != FBDEV_EXT_MAGIC)
            __debugbreak();

        KLInf("Going to delete Device %p IrpCount %d", Device, DevExt->IrpCount);
        {
            int j;
            
            for (j = 0; j < ARRAY_SIZE(DevExt->IrpMjCount); j++) {
                if (DevExt->IrpMjCount[j] != 0) {
                    KLInf("Device %p IrpMjCount[0x%x]=%d", Device, j, DevExt->IrpMjCount[j]);
                }
            }
        }

        if (DevExt->SymLinkCreated) {
            KLInf("Deleting Device %p symlink %wZ", Device, &DevExt->SymLinkName);
            IoDeleteSymbolicLink(&DevExt->SymLinkName);
        }
        if (DevExt->AttachedToDevice) {
            ObDereferenceObject(DevExt->TargetDevice);
            ObDereferenceObject(DevExt->AttachedToDevice);
        }
        IoDeleteDevice(Device);
        ObDereferenceObject(Device);
    }

    NpFree(DeviceObjectList, MTAG_DRV);

    KLInf("UnloadRoutine completed");
    KLogDeinit();
}

NTSTATUS DevCtlCtlHandler( IN PDEVICE_OBJECT Device, IN PIRP Irp )
{
    NTSTATUS Status = STATUS_SUCCESS;
    PIO_STACK_LOCATION IrpStack = IoGetCurrentIrpStackLocation(Irp);
    ULONG ControlCode = IrpStack->Parameters.DeviceIoControl.IoControlCode;
    ULONG method = ControlCode & 0x3;
    ULONG ResultLength = 0;
    ULONG InputLength = IrpStack->Parameters.DeviceIoControl.InputBufferLength;
    ULONG OutputLength = IrpStack->Parameters.DeviceIoControl.OutputBufferLength;
    PVOID Buffer = Irp->AssociatedIrp.SystemBuffer;

    KeEnterGuardedRegion();
    KLInf("Device %p, Ctl 0x%x", Device, ControlCode);
    if (OutputLength < InputLength) {
        KLErr("Invalid OutputLen 0x%x vs InputLen 0x%x", OutputLength, InputLength);
        Status = STATUS_INVALID_PARAMETER;
        goto Done;
    }

    switch (ControlCode) {
    case IOCTL_FBACKUP_INIT:  
        Status = FbCtlInit();
        break;
    case IOCTL_FBACKUP_RELEASE:
        Status = FbCtlRelease();
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
    KeLeaveGuardedRegion();
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
    FbFltPassThrough(PFBDEV_EXT DevExt, PIRP Irp)
{
    IoSkipCurrentIrpStackLocation(Irp);
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
        return FbFltPassThrough(DevExt, Irp);

    return Status;
}

NTSTATUS
DriverIrpHandler(IN PDEVICE_OBJECT DeviceObject, IN PIRP Irp)
{
    PFBDEV_EXT DevExt;
    PFBDRIVER FbDriver = GetFbDriver();
    NTSTATUS Status;

    UnloadProtectionAcquire(&FbDriver->UnloadProtection);

    DevExt = (PFBDEV_EXT)DeviceObject->DeviceExtension;
    if (DevExt->Magic != FBDEV_EXT_MAGIC)
        __debugbreak();

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
    NTSTATUS Status = STATUS_SUCCESS;
    PDEVICE_OBJECT  DeviceObject = NULL;
    UNICODE_STRING  DeviceName;
    ULONG i;

    DPRINT("DriverEntry Driver %p, RegPath %wZ\n", DriverObject, RegistryPath);

    FbDriverInit();
    DriverObject->DriverUnload = DriverUnloadRoutine;
    for (i = 0; i <= IRP_MJ_MAXIMUM_FUNCTION; i++) {
        DriverObject->MajorFunction[i] = DriverIrpHandler;
    }
    DriverObject->FastIoDispatch = &g_FbFastIoDispatch;
    RtlInitUnicodeString( &DeviceName, FBACKUP_DRV_NT_DEVICE_NAME_W);

    Status = KLogInit();
    if (!NT_SUCCESS(Status)) {
        DPRINT("DriverLibInit failure\n");
        return Status;
    }

    KLInf("Driver %p, RegPath %wZ", DriverObject, RegistryPath);

    Status = IoCreateDevice(DriverObject,
        sizeof(FBDEV_EXT),
        &DeviceName,
        FILE_DEVICE_UNKNOWN,
        0,
        FALSE, 
        &DeviceObject);

    if(!NT_SUCCESS(Status)) {
        KLogDeinit();
        return Status;
    }

    PFBDEV_EXT DevExt = (PFBDEV_EXT)DeviceObject->DeviceExtension;
    RtlZeroMemory(DevExt, sizeof(*DevExt));
    DevExt->DeviceObject = DeviceObject;
    DevExt->Magic = FBDEV_EXT_MAGIC;

    KLInf("Device %p, DevExt %p", DeviceObject, DevExt);

    RtlInitUnicodeString(&DevExt->SymLinkName, FBACKUP_DRV_DOS_DEVICE_NAME_W );

    Status = IoCreateSymbolicLink( &DevExt->SymLinkName, &DeviceName );
    if (!NT_SUCCESS(Status)) { 
        IoDeleteDevice( DeviceObject );
        KLogDeinit();
        return Status;
    }
    DevExt->SymLinkCreated = TRUE;
    GetFbDriver()->CtlDevice = DeviceObject;
    GetFbDriver()->Driver = DriverObject;
    KLInf("DriverEntry Status 0x%x", Status);

    return Status;
}
