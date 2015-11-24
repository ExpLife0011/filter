#include "inc/driver.h"
#include "inc/klog.h"
#include "inc/mtags.h"
#include "h/ioctl.h"

#define FBDEV_EXT_MAGIC 0xCBDACBDA

typedef struct _FBDEV_EXT
{
    ULONG Magic;
    PDEVICE_OBJECT DeviceObject;
    UNICODE_STRING SymLinkName;
    BOOLEAN SymLinkCreated;
    PDEVICE_OBJECT AttachedDevice;
} FBDEV_EXT, *PFBDEV_EXT;

PDRIVER_OBJECT g_Driver;
PDEVICE_OBJECT g_CtlDevice;

NTSTATUS 
    CompleteIrp( PIRP Irp, NTSTATUS status, ULONG info)
{
    Irp->IoStatus.Status = status;
    Irp->IoStatus.Information = info;
    IoCompleteRequest(Irp,IO_NO_INCREMENT);
    return status;
}

VOID DriverUnloadRoutine(IN PDRIVER_OBJECT DriverObject)
{
    ULONG NrDeviceObjects, DeviceObjectListSize;
    PDEVICE_OBJECT *DeviceObjectList, Device;
    NTSTATUS Status;
    ULONG i;
    PFBDEV_EXT DevExt;

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
            DevExt = (PFBDEV_EXT)Device->DeviceExtension;
            if (DevExt->Magic != FBDEV_EXT_MAGIC)
                __debugbreak();
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
DriverIrpHandler(IN PDEVICE_OBJECT DeviceObject, IN PIRP Irp)
{
    PFBDEV_EXT DevExt;

    DevExt = (PFBDEV_EXT)DeviceObject->DeviceExtension;
    if (DevExt->Magic != FBDEV_EXT_MAGIC)
        __debugbreak();

    if (DeviceObject == g_CtlDevice)
        return DevCtlIrpHandler(DeviceObject, Irp);

    if (DevExt->AttachedDevice) {
        IoSkipCurrentIrpStackLocation(Irp);
        return IoCallDriver(DevExt->AttachedDevice, Irp);
    }

    KLErr("Unhandled Device %p Irp %p\n", DeviceObject, Irp);
    return CompleteIrp(Irp, STATUS_NOT_SUPPORTED, 0);
}

NTSTATUS DriverEntry(IN PDRIVER_OBJECT DriverObject,
                     IN PUNICODE_STRING RegistryPath)
{
    NTSTATUS Status = STATUS_SUCCESS;
    PDEVICE_OBJECT  DeviceObject = NULL;
    UNICODE_STRING  DeviceName;
    ULONG i;

    DPRINT("DriverEntry Driver %p, RegPath %wZ\n", DriverObject, RegistryPath);

    DriverObject->DriverUnload = DriverUnloadRoutine;
    for (i = 0; i <= IRP_MJ_MAXIMUM_FUNCTION; i++) {
        DriverObject->MajorFunction[i] = DriverIrpHandler;
    }

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
    g_CtlDevice = DeviceObject;
    g_Driver = DriverObject;
    KLInf("DriverEntry Status 0x%x", Status);

    return Status;
}
