#include "inc/driver.h"
#include "inc/klog.h"
#include "inc/mtags.h"
#include "h/ioctl.h"

typedef struct _DRV_DEVICE_EXTENSION
{
    PDEVICE_OBJECT DeviceObject;
    UNICODE_STRING SymLinkName;
    BOOLEAN SymLinkCreated;
} DRV_DEVICE_EXTENSION, *PDRV_DEVICE_EXTENSION;

NTSTATUS 
    CompleteIrp( PIRP Irp, NTSTATUS status, ULONG info)
{
    Irp->IoStatus.Status = status;
    Irp->IoStatus.Information = info;
    IoCompleteRequest(Irp,IO_NO_INCREMENT);
    return status;
}

VOID UnloadRoutine(IN PDRIVER_OBJECT DriverObject)
{
    ULONG NrDeviceObjects, DeviceObjectListSize;
    PDEVICE_OBJECT *DeviceObjectList, Device;
    NTSTATUS Status;
    ULONG i;
    PDRV_DEVICE_EXTENSION DevExt;

    KLInf("UnloadRoutine driver %p", DriverObject);

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

        for (i = 0; i < NrDeviceObjects; i++) {
            Device = DeviceObjectList[i];
            KLInf("Going to delete device %p", Device);
            DevExt = (PDRV_DEVICE_EXTENSION)Device->DeviceExtension;
            if (DevExt->SymLinkCreated) {
                KLInf("Deleting symlink %wZ", &DevExt->SymLinkName);
                IoDeleteSymbolicLink(&DevExt->SymLinkName);
            }
            IoDeleteDevice(Device);
            ObDereferenceObject(Device);
        }
        NpFree(DeviceObjectList, MTAG_DRV);
        break;
    }
    KLInf("UnloadRoutine completed");
    KLogDeinit();
}

NTSTATUS DriverDeviceControlHandler( IN PDEVICE_OBJECT Device, IN PIRP Irp )
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
        Status = STATUS_SUCCESS;
        break;
    case IOCTL_FBACKUP_RELEASE:
        Status = STATUS_SUCCESS;
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
DriverIrpHandler(IN PDEVICE_OBJECT DeviceObject, IN PIRP Irp)
{
    NTSTATUS Status;
    PIO_STACK_LOCATION CurrentIrpStack = IoGetCurrentIrpStackLocation(Irp);

    KLInf("DevObj %p MJ 0x%x MN 0x%x",
        DeviceObject, CurrentIrpStack->MajorFunction, CurrentIrpStack->MinorFunction);

    switch (CurrentIrpStack->MajorFunction) {
    case IRP_MJ_DEVICE_CONTROL:
        Status = DriverDeviceControlHandler(DeviceObject, Irp);
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

NTSTATUS DriverEntry(IN PDRIVER_OBJECT DriverObject,
                     IN PUNICODE_STRING RegistryPath)
{
    NTSTATUS Status = STATUS_SUCCESS;
    PDEVICE_OBJECT  DeviceObject = NULL;
    UNICODE_STRING  DeviceName;

    DPRINT("DriverEntry Driver %p, RegPath %wZ\n", DriverObject, RegistryPath);

    DriverObject->DriverUnload = UnloadRoutine;
    DriverObject->MajorFunction[IRP_MJ_CREATE]= DriverIrpHandler;
    DriverObject->MajorFunction[IRP_MJ_CLOSE] = DriverIrpHandler;
    DriverObject->MajorFunction[IRP_MJ_READ]  = DriverIrpHandler;
    DriverObject->MajorFunction[IRP_MJ_WRITE] = DriverIrpHandler;
    DriverObject->MajorFunction[IRP_MJ_DEVICE_CONTROL]= DriverIrpHandler;
    DriverObject->MajorFunction[IRP_MJ_FLUSH_BUFFERS]  = DriverIrpHandler;
    DriverObject->MajorFunction[IRP_MJ_CLEANUP] = DriverIrpHandler;
    DriverObject->MajorFunction[IRP_MJ_POWER] = DriverIrpHandler;

    RtlInitUnicodeString( &DeviceName, FBACKUP_DRV_NT_DEVICE_NAME_W);

    Status = KLogInit();
    if (!NT_SUCCESS(Status)) {
        DPRINT("DriverLibInit failure\n");
        return Status;
    }

    KLInf("Driver %p, RegPath %wZ", DriverObject, RegistryPath);

    Status = IoCreateDevice(DriverObject,
        sizeof(DRV_DEVICE_EXTENSION),
        &DeviceName,
        FILE_DEVICE_UNKNOWN,
        0,
        FALSE, 
        &DeviceObject);

    if(!NT_SUCCESS(Status)) {
        KLogDeinit();
        return Status;
    }

    PDRV_DEVICE_EXTENSION DevExt = (PDRV_DEVICE_EXTENSION)DeviceObject->DeviceExtension;
    RtlZeroMemory(DevExt, sizeof(*DevExt));
    DevExt->DeviceObject = DeviceObject;

    KLInf("Device %p, DevExt %p", DeviceObject, DevExt);

    RtlInitUnicodeString(&DevExt->SymLinkName, FBACKUP_DRV_DOS_DEVICE_NAME_W );

    Status = IoCreateSymbolicLink( &DevExt->SymLinkName, &DeviceName );
    if (!NT_SUCCESS(Status)) { 
        IoDeleteDevice( DeviceObject );
        KLogDeinit();
        return Status;
    }
    DevExt->SymLinkCreated = TRUE;
    KLInf("DriverEntry Status 0x%x", Status);

    return Status;
}
