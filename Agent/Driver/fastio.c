#include "inc\fastio.h"
#include "inc\klog.h"
#include "inc\driver.h"

#define VALID_FAST_IO_DISPATCH_HANDLER(_FastIoDispatchPtr, _FieldName) \
    (((_FastIoDispatchPtr) != NULL) && \
    (((_FastIoDispatchPtr)->SizeOfFastIoDispatch) >= \
    (FIELD_OFFSET(FAST_IO_DISPATCH, _FieldName) + sizeof(void *))) && \
    ((_FastIoDispatchPtr)->_FieldName != NULL))

BOOLEAN
FbFastIoCheckIfPossible(
    _In_ PFILE_OBJECT FileObject,
    _In_ PLARGE_INTEGER FileOffset,
    _In_ ULONG Length,
    _In_ BOOLEAN Wait,
    _In_ ULONG LockKey,
    _In_ BOOLEAN CheckForReadOperation,
    PIO_STATUS_BLOCK IoStatus,
    _In_ PDEVICE_OBJECT DeviceObject
    )
{
    PFBDEV_EXT DevExt;
    PFBDRIVER FbDriver = GetFbDriver();
    BOOLEAN Result = FALSE;
    PFAST_IO_DISPATCH FastIoDispatch;
    PDEVICE_OBJECT NextDevice;

    UnloadProtectionAcquire(&FbDriver->UnloadProtection);
    if (DeviceObject == FbDriver->CtlDevice)
        goto UnlockReturn;

    KLDbg("FastIo");

    DevExt = (PFBDEV_EXT)DeviceObject->DeviceExtension;
    if (DevExt->Magic != FBDEV_EXT_MAGIC)
        __debugbreak();
    InterlockedIncrement(&DevExt->FastIoCount);
    InterlockedIncrement(&DevExt->FastIoCountByIndex[FastIoCheckIfPossibleIndex]);
    NextDevice = DevExt->AttachedToDevice;
    FastIoDispatch = NextDevice->DriverObject->FastIoDispatch;
    if (VALID_FAST_IO_DISPATCH_HANDLER(FastIoDispatch, FastIoCheckIfPossible)) {
        Result = FastIoDispatch->FastIoCheckIfPossible(FileObject, FileOffset, Length,
                                                       Wait, LockKey, CheckForReadOperation,
                                                       IoStatus, NextDevice);
    }
    if (Result)
        InterlockedIncrement(&DevExt->FastIoSuccessCount);
    if (Result)
        InterlockedIncrement(&DevExt->FastIoSuccessCountByIndex[FastIoCheckIfPossibleIndex]);
UnlockReturn:
    UnloadProtectionRelease(&FbDriver->UnloadProtection);
    return Result;
}

BOOLEAN
FbFastIoRead(
    _In_ PFILE_OBJECT FileObject,
    _In_ PLARGE_INTEGER FileOffset,
    _In_ ULONG Length,
    _In_ BOOLEAN Wait,
    _In_ ULONG LockKey,
    _Out_ PVOID Buffer,
    _Out_ PIO_STATUS_BLOCK IoStatus,
    _In_ PDEVICE_OBJECT DeviceObject
    )
{
    PFBDEV_EXT DevExt;
    PFBDRIVER FbDriver = GetFbDriver();
    BOOLEAN Result = FALSE;
    PFAST_IO_DISPATCH FastIoDispatch;
    PDEVICE_OBJECT NextDevice;

    UnloadProtectionAcquire(&FbDriver->UnloadProtection);
    if (DeviceObject == FbDriver->CtlDevice)
        goto UnlockReturn;

    KLDbg("FastIo");

    DevExt = (PFBDEV_EXT)DeviceObject->DeviceExtension;
    if (DevExt->Magic != FBDEV_EXT_MAGIC)
        __debugbreak();
    InterlockedIncrement(&DevExt->FastIoCount);
    InterlockedIncrement(&DevExt->FastIoCountByIndex[FastIoReadIndex]);
    NextDevice = DevExt->AttachedToDevice;
    FastIoDispatch = NextDevice->DriverObject->FastIoDispatch;
    if (VALID_FAST_IO_DISPATCH_HANDLER(FastIoDispatch, FastIoRead)) {
        Result = FastIoDispatch->FastIoRead(FileObject, FileOffset, Length,
                                            Wait, LockKey, Buffer, IoStatus, NextDevice);
    }
    if (Result)
        InterlockedIncrement(&DevExt->FastIoSuccessCount);
    if (Result)
        InterlockedIncrement(&DevExt->FastIoSuccessCountByIndex[FastIoReadIndex]);
UnlockReturn:
    UnloadProtectionRelease(&FbDriver->UnloadProtection);
    return Result;
}

BOOLEAN
FbFastIoWrite(
    _In_ PFILE_OBJECT FileObject,
    _In_ PLARGE_INTEGER FileOffset,
    _In_ ULONG Length,
    _In_ BOOLEAN Wait,
    _In_ ULONG LockKey,
    _In_ PVOID Buffer,
    _Out_ PIO_STATUS_BLOCK IoStatus,
    _In_ PDEVICE_OBJECT DeviceObject
    )
{
    PFBDEV_EXT DevExt;
    PFBDRIVER FbDriver = GetFbDriver();
    BOOLEAN Result = FALSE;
    PFAST_IO_DISPATCH FastIoDispatch;
    PDEVICE_OBJECT NextDevice;

    UnloadProtectionAcquire(&FbDriver->UnloadProtection);
    if (DeviceObject == FbDriver->CtlDevice)
        goto UnlockReturn;

    KLDbg("FastIo");

    DevExt = (PFBDEV_EXT)DeviceObject->DeviceExtension;
    if (DevExt->Magic != FBDEV_EXT_MAGIC)
        __debugbreak();
    InterlockedIncrement(&DevExt->FastIoCount);
    InterlockedIncrement(&DevExt->FastIoCountByIndex[FastIoWriteIndex]);
    NextDevice = DevExt->AttachedToDevice;
    FastIoDispatch = NextDevice->DriverObject->FastIoDispatch;
    if (VALID_FAST_IO_DISPATCH_HANDLER(FastIoDispatch, FastIoWrite)) {
        KLInf("FastIoWrite: File %p %wZ Offset 0x%llx Length 0x%x",
              FileObject, &FileObject->FileName, FileOffset->QuadPart, Length);
        Result = FastIoDispatch->FastIoWrite(FileObject, FileOffset, Length,
                                             Wait, LockKey, Buffer, IoStatus, NextDevice);
    }
    if (Result)
        InterlockedIncrement(&DevExt->FastIoSuccessCount);
    if (Result)
        InterlockedIncrement(&DevExt->FastIoSuccessCountByIndex[FastIoWriteIndex]);
UnlockReturn:
    UnloadProtectionRelease(&FbDriver->UnloadProtection);
    return Result;
}

BOOLEAN
FbFastIoQueryBasicInfo(
    _In_ PFILE_OBJECT FileObject,
    _In_ BOOLEAN Wait,
    _Out_ PFILE_BASIC_INFORMATION Buffer,
    _Out_ PIO_STATUS_BLOCK IoStatus,
    _In_ PDEVICE_OBJECT DeviceObject
    )
{
    PFBDEV_EXT DevExt;
    PFBDRIVER FbDriver = GetFbDriver();
    BOOLEAN Result = FALSE;
    PFAST_IO_DISPATCH FastIoDispatch;
    PDEVICE_OBJECT NextDevice;

    UnloadProtectionAcquire(&FbDriver->UnloadProtection);
    if (DeviceObject == FbDriver->CtlDevice)
        goto UnlockReturn;

    KLDbg("FastIo");

    DevExt = (PFBDEV_EXT)DeviceObject->DeviceExtension;
    if (DevExt->Magic != FBDEV_EXT_MAGIC)
        __debugbreak();
    InterlockedIncrement(&DevExt->FastIoCount);
    InterlockedIncrement(&DevExt->FastIoCountByIndex[FastIoQueryBasicInfoIndex]);
    NextDevice = DevExt->AttachedToDevice;
    FastIoDispatch = NextDevice->DriverObject->FastIoDispatch;
    if (VALID_FAST_IO_DISPATCH_HANDLER(FastIoDispatch, FastIoQueryBasicInfo)) {
        Result = FastIoDispatch->FastIoQueryBasicInfo(FileObject, Wait, Buffer, IoStatus,
                                                      NextDevice);
    }
    if (Result)
        InterlockedIncrement(&DevExt->FastIoSuccessCount);
    if (Result)
        InterlockedIncrement(&DevExt->FastIoSuccessCountByIndex[FastIoQueryBasicInfoIndex]);
UnlockReturn:
    UnloadProtectionRelease(&FbDriver->UnloadProtection);
    return Result;
}

BOOLEAN
FbFastIoQueryStandardInfo(
    _In_ PFILE_OBJECT FileObject,
    _In_ BOOLEAN Wait,
    _Out_ PFILE_STANDARD_INFORMATION Buffer,
    _Out_ PIO_STATUS_BLOCK IoStatus,
    _In_ PDEVICE_OBJECT DeviceObject
    )
{
    PFBDEV_EXT DevExt;
    PFBDRIVER FbDriver = GetFbDriver();
    BOOLEAN Result = FALSE;
    PFAST_IO_DISPATCH FastIoDispatch;
    PDEVICE_OBJECT NextDevice;

    UnloadProtectionAcquire(&FbDriver->UnloadProtection);
    if (DeviceObject == FbDriver->CtlDevice)
        goto UnlockReturn;

    KLDbg("FastIo");

    DevExt = (PFBDEV_EXT)DeviceObject->DeviceExtension;
    if (DevExt->Magic != FBDEV_EXT_MAGIC)
        __debugbreak();
    InterlockedIncrement(&DevExt->FastIoCount);
    InterlockedIncrement(&DevExt->FastIoCountByIndex[FastIoQueryStandardInfoIndex]);
    NextDevice = DevExt->AttachedToDevice;
    FastIoDispatch = NextDevice->DriverObject->FastIoDispatch;
    if (VALID_FAST_IO_DISPATCH_HANDLER(FastIoDispatch, FastIoQueryStandardInfo)) {
        Result = FastIoDispatch->FastIoQueryStandardInfo(FileObject, Wait, Buffer, IoStatus,
                                                         NextDevice);
    }
    if (Result)
        InterlockedIncrement(&DevExt->FastIoSuccessCount);
    if (Result)
        InterlockedIncrement(&DevExt->FastIoSuccessCountByIndex[FastIoQueryStandardInfoIndex]);
UnlockReturn:
    UnloadProtectionRelease(&FbDriver->UnloadProtection);
    return Result;
}

BOOLEAN
FbFastIoLock (
    _In_ PFILE_OBJECT FileObject,
    _In_ PLARGE_INTEGER FileOffset,
    _In_ PLARGE_INTEGER Length,
    _In_ PEPROCESS ProcessId,
    _In_ ULONG Key,
    _In_ BOOLEAN FailImmediately,
    _In_ BOOLEAN ExclusiveLock,
    _Out_ PIO_STATUS_BLOCK IoStatus,
    _In_ PDEVICE_OBJECT DeviceObject
    )
{
    PFBDEV_EXT DevExt;
    PFBDRIVER FbDriver = GetFbDriver();
    BOOLEAN Result = FALSE;
    PFAST_IO_DISPATCH FastIoDispatch;
    PDEVICE_OBJECT NextDevice;

    UnloadProtectionAcquire(&FbDriver->UnloadProtection);
    if (DeviceObject == FbDriver->CtlDevice)
        goto UnlockReturn;

    KLDbg("FastIo");

    DevExt = (PFBDEV_EXT)DeviceObject->DeviceExtension;
    if (DevExt->Magic != FBDEV_EXT_MAGIC)
        __debugbreak();
    InterlockedIncrement(&DevExt->FastIoCount);
    InterlockedIncrement(&DevExt->FastIoCountByIndex[FastIoLockIndex]);
    NextDevice = DevExt->AttachedToDevice;
    FastIoDispatch = NextDevice->DriverObject->FastIoDispatch;
    if (VALID_FAST_IO_DISPATCH_HANDLER(FastIoDispatch, FastIoLock)) {
        Result = FastIoDispatch->FastIoLock(FileObject, FileOffset, Length, ProcessId, Key,
                                            FailImmediately, ExclusiveLock, IoStatus, NextDevice);
    }
    if (Result)
        InterlockedIncrement(&DevExt->FastIoSuccessCount);
    if (Result)
        InterlockedIncrement(&DevExt->FastIoSuccessCountByIndex[FastIoLockIndex]);
UnlockReturn:
    UnloadProtectionRelease(&FbDriver->UnloadProtection);
    return Result;
}

BOOLEAN
FbFastIoUnlockSingle (
    _In_ PFILE_OBJECT FileObject,
    _In_ PLARGE_INTEGER FileOffset,
    _In_ PLARGE_INTEGER Length,
    _In_ PEPROCESS ProcessId,
    _In_ ULONG Key,
    _Out_ PIO_STATUS_BLOCK IoStatus,
    _In_ PDEVICE_OBJECT DeviceObject
    )
{
    PFBDEV_EXT DevExt;
    PFBDRIVER FbDriver = GetFbDriver();
    BOOLEAN Result = FALSE;
    PFAST_IO_DISPATCH FastIoDispatch;
    PDEVICE_OBJECT NextDevice;

    UnloadProtectionAcquire(&FbDriver->UnloadProtection);
    if (DeviceObject == FbDriver->CtlDevice)
        goto UnlockReturn;

    KLDbg("FastIo");

    DevExt = (PFBDEV_EXT)DeviceObject->DeviceExtension;
    if (DevExt->Magic != FBDEV_EXT_MAGIC)
        __debugbreak();
    InterlockedIncrement(&DevExt->FastIoCount);
    InterlockedIncrement(&DevExt->FastIoCountByIndex[FastIoUnlockSingleIndex]);
    NextDevice = DevExt->AttachedToDevice;
    FastIoDispatch = NextDevice->DriverObject->FastIoDispatch;
    if (VALID_FAST_IO_DISPATCH_HANDLER(FastIoDispatch, FastIoUnlockSingle)) {
        Result = FastIoDispatch->FastIoUnlockSingle(FileObject, FileOffset, Length, ProcessId, Key,
                                                    IoStatus, NextDevice);
    }
    if (Result)
        InterlockedIncrement(&DevExt->FastIoSuccessCount);
    if (Result)
        InterlockedIncrement(&DevExt->FastIoSuccessCountByIndex[FastIoUnlockSingleIndex]);
UnlockReturn:
    UnloadProtectionRelease(&FbDriver->UnloadProtection);
    return Result;
}

BOOLEAN
FbFastIoUnlockAll(
    _In_ PFILE_OBJECT FileObject,
    _In_ PEPROCESS ProcessId,
    _Out_ PIO_STATUS_BLOCK IoStatus,
    _In_ PDEVICE_OBJECT DeviceObject
    )
{
    PFBDEV_EXT DevExt;
    PFBDRIVER FbDriver = GetFbDriver();
    BOOLEAN Result = FALSE;
    PFAST_IO_DISPATCH FastIoDispatch;
    PDEVICE_OBJECT NextDevice;

    UnloadProtectionAcquire(&FbDriver->UnloadProtection);
    if (DeviceObject == FbDriver->CtlDevice)
        goto UnlockReturn;

    KLDbg("FastIo");

    DevExt = (PFBDEV_EXT)DeviceObject->DeviceExtension;
    if (DevExt->Magic != FBDEV_EXT_MAGIC)
        __debugbreak();
    InterlockedIncrement(&DevExt->FastIoCount);
    InterlockedIncrement(&DevExt->FastIoCountByIndex[FastIoUnlockAllIndex]);
    NextDevice = DevExt->AttachedToDevice;
    FastIoDispatch = NextDevice->DriverObject->FastIoDispatch;
    if (VALID_FAST_IO_DISPATCH_HANDLER(FastIoDispatch, FastIoUnlockAll)) {
        Result = FastIoDispatch->FastIoUnlockAll(FileObject, ProcessId,
                                                 IoStatus, NextDevice);
    }
    if (Result)
        InterlockedIncrement(&DevExt->FastIoSuccessCount);
    if (Result)
        InterlockedIncrement(&DevExt->FastIoSuccessCountByIndex[FastIoUnlockAllIndex]);
UnlockReturn:
    UnloadProtectionRelease(&FbDriver->UnloadProtection);
    return Result;
}

BOOLEAN
FbFastIoUnlockAllByKey(
    _In_ PFILE_OBJECT FileObject,
    _In_ PVOID ProcessId,
    _In_ ULONG Key,
    _Out_ PIO_STATUS_BLOCK IoStatus,
    _In_ PDEVICE_OBJECT DeviceObject
    )
{
    PFBDEV_EXT DevExt;
    PFBDRIVER FbDriver = GetFbDriver();
    BOOLEAN Result = FALSE;
    PFAST_IO_DISPATCH FastIoDispatch;
    PDEVICE_OBJECT NextDevice;

    UnloadProtectionAcquire(&FbDriver->UnloadProtection);
    if (DeviceObject == FbDriver->CtlDevice)
        goto UnlockReturn;

    KLDbg("FastIo");

    DevExt = (PFBDEV_EXT)DeviceObject->DeviceExtension;
    if (DevExt->Magic != FBDEV_EXT_MAGIC)
        __debugbreak();
    InterlockedIncrement(&DevExt->FastIoCount);
    InterlockedIncrement(&DevExt->FastIoCountByIndex[FastIoUnlockAllByKeyIndex]);
    NextDevice = DevExt->AttachedToDevice;
    FastIoDispatch = NextDevice->DriverObject->FastIoDispatch;
    if (VALID_FAST_IO_DISPATCH_HANDLER(FastIoDispatch, FastIoUnlockAllByKey)) {
        Result = FastIoDispatch->FastIoUnlockAllByKey(FileObject, ProcessId, Key,
                                                      IoStatus, NextDevice);
    }
    if (Result)
        InterlockedIncrement(&DevExt->FastIoSuccessCount);
    if (Result)
        InterlockedIncrement(&DevExt->FastIoSuccessCountByIndex[FastIoUnlockAllByKeyIndex]);
UnlockReturn:
    UnloadProtectionRelease(&FbDriver->UnloadProtection);
    return Result;
}

BOOLEAN
FbFastIoDeviceControl(
    _In_ PFILE_OBJECT FileObject,
    _In_ BOOLEAN Wait,
    _In_opt_ PVOID InputBuffer,
    _In_ ULONG InputBufferLength,
    _Out_opt_ PVOID OutputBuffer,
    _In_ ULONG OutputBufferLength,
    _In_ ULONG IoControlCode,
    _Out_ PIO_STATUS_BLOCK IoStatus,
    _In_ PDEVICE_OBJECT DeviceObject
    )
{
    PFBDEV_EXT DevExt;
    PFBDRIVER FbDriver = GetFbDriver();
    BOOLEAN Result = FALSE;
    PFAST_IO_DISPATCH FastIoDispatch;
    PDEVICE_OBJECT NextDevice;

    UnloadProtectionAcquire(&FbDriver->UnloadProtection);
    if (DeviceObject == FbDriver->CtlDevice)
        goto UnlockReturn;

    KLDbg("FastIo");

    DevExt = (PFBDEV_EXT)DeviceObject->DeviceExtension;
    if (DevExt->Magic != FBDEV_EXT_MAGIC)
        __debugbreak();
    InterlockedIncrement(&DevExt->FastIoCount);
    InterlockedIncrement(&DevExt->FastIoCountByIndex[FastIoDeviceControlIndex]);
    NextDevice = DevExt->AttachedToDevice;
    FastIoDispatch = NextDevice->DriverObject->FastIoDispatch;
    if (VALID_FAST_IO_DISPATCH_HANDLER(FastIoDispatch, FastIoDeviceControl)) {
        Result = FastIoDispatch->FastIoDeviceControl(FileObject, Wait, InputBuffer, InputBufferLength,
                                                     OutputBuffer, OutputBufferLength, IoControlCode, IoStatus,
                                                     NextDevice);
    }
    if (Result)
        InterlockedIncrement(&DevExt->FastIoSuccessCount);
    if (Result)
        InterlockedIncrement(&DevExt->FastIoSuccessCountByIndex[FastIoDeviceControlIndex]);
UnlockReturn:
    UnloadProtectionRelease(&FbDriver->UnloadProtection);
    return Result;
}

VOID
FbFastIoAcquireFile(
    _In_ PFILE_OBJECT FileObject
    )
{
    KLErr("Unimplemented");
    return;
}

VOID
FbFastIoReleaseFile(
    _In_ PFILE_OBJECT FileObject
    )
{
    KLErr("Unimplemented");
    return;
}

VOID
FbFastIoDetachDevice(
    _In_ PDEVICE_OBJECT SourceDevice,
    _In_ PDEVICE_OBJECT TargetDevice
    )
{
    KLErr("Unimplemented");
    return;
}

BOOLEAN
FbFastIoQueryNetworkOpenInfo(
    _In_ PFILE_OBJECT FileObject,
    _In_ BOOLEAN Wait,
    _Out_ PFILE_NETWORK_OPEN_INFORMATION Buffer,
    _Out_ PIO_STATUS_BLOCK IoStatus,
    _In_ PDEVICE_OBJECT DeviceObject
    )
{
    PFBDEV_EXT DevExt;
    PFBDRIVER FbDriver = GetFbDriver();
    BOOLEAN Result = FALSE;
    PFAST_IO_DISPATCH FastIoDispatch;
    PDEVICE_OBJECT NextDevice;

    UnloadProtectionAcquire(&FbDriver->UnloadProtection);
    if (DeviceObject == FbDriver->CtlDevice)
        goto UnlockReturn;

    KLDbg("FastIo");

    DevExt = (PFBDEV_EXT)DeviceObject->DeviceExtension;
    if (DevExt->Magic != FBDEV_EXT_MAGIC)
        __debugbreak();
    InterlockedIncrement(&DevExt->FastIoCount);
    InterlockedIncrement(&DevExt->FastIoCountByIndex[FastIoQueryNetworkOpenInfoIndex]);
    NextDevice = DevExt->AttachedToDevice;
    FastIoDispatch = NextDevice->DriverObject->FastIoDispatch;
    if (VALID_FAST_IO_DISPATCH_HANDLER(FastIoDispatch, FastIoQueryNetworkOpenInfo)) {
        Result = FastIoDispatch->FastIoQueryNetworkOpenInfo(FileObject, Wait, Buffer,
                                                            IoStatus, NextDevice);
    }
    if (Result)
        InterlockedIncrement(&DevExt->FastIoSuccessCount);
    if (Result)
        InterlockedIncrement(&DevExt->FastIoSuccessCountByIndex[FastIoQueryNetworkOpenInfoIndex]);
UnlockReturn:
    UnloadProtectionRelease(&FbDriver->UnloadProtection);
    return Result;
}

BOOLEAN
FbFastIoMdlRead(
    _In_ PFILE_OBJECT FileObject,
    _In_ PLARGE_INTEGER FileOffset,
    _In_ ULONG Length,
    _In_ ULONG LockKey,
    _Out_ PMDL *MdlChain,
    _Out_ PIO_STATUS_BLOCK IoStatus,
    _In_ PDEVICE_OBJECT DeviceObject
    )
{
    PFBDEV_EXT DevExt;
    PFBDRIVER FbDriver = GetFbDriver();
    BOOLEAN Result = FALSE;
    PFAST_IO_DISPATCH FastIoDispatch;
    PDEVICE_OBJECT NextDevice;

    UnloadProtectionAcquire(&FbDriver->UnloadProtection);
    if (DeviceObject == FbDriver->CtlDevice)
        goto UnlockReturn;

    KLDbg("FastIo");

    DevExt = (PFBDEV_EXT)DeviceObject->DeviceExtension;
    if (DevExt->Magic != FBDEV_EXT_MAGIC)
        __debugbreak();
    InterlockedIncrement(&DevExt->FastIoCount);
    InterlockedIncrement(&DevExt->FastIoCountByIndex[FastIoMdlReadIndex]);
    NextDevice = DevExt->AttachedToDevice;
    FastIoDispatch = NextDevice->DriverObject->FastIoDispatch;
    if (VALID_FAST_IO_DISPATCH_HANDLER(FastIoDispatch, MdlRead)) {
        Result = FastIoDispatch->MdlRead(FileObject, FileOffset, Length, LockKey, MdlChain,
                                         IoStatus, NextDevice);
    }
    if (Result)
        InterlockedIncrement(&DevExt->FastIoSuccessCount);
    if (Result)
        InterlockedIncrement(&DevExt->FastIoSuccessCountByIndex[FastIoMdlReadIndex]);
UnlockReturn:
    UnloadProtectionRelease(&FbDriver->UnloadProtection);
    return Result;
}

BOOLEAN
FbFastIoMdlReadComplete(
    _In_ PFILE_OBJECT FileObject,
    _In_ PMDL MdlChain,
    _In_ PDEVICE_OBJECT DeviceObject
    )
{
    PFBDEV_EXT DevExt;
    PFBDRIVER FbDriver = GetFbDriver();
    BOOLEAN Result = FALSE;
    PFAST_IO_DISPATCH FastIoDispatch;
    PDEVICE_OBJECT NextDevice;

    UnloadProtectionAcquire(&FbDriver->UnloadProtection);
    if (DeviceObject == FbDriver->CtlDevice)
        goto UnlockReturn;

    KLDbg("FastIo");

    DevExt = (PFBDEV_EXT)DeviceObject->DeviceExtension;
    if (DevExt->Magic != FBDEV_EXT_MAGIC)
        __debugbreak();
    InterlockedIncrement(&DevExt->FastIoCount);
    InterlockedIncrement(&DevExt->FastIoCountByIndex[FastIoMdlReadCompleteIndex]);
    NextDevice = DevExt->AttachedToDevice;
    FastIoDispatch = NextDevice->DriverObject->FastIoDispatch;
    if (VALID_FAST_IO_DISPATCH_HANDLER(FastIoDispatch, MdlReadComplete)) {
        Result = FastIoDispatch->MdlReadComplete(FileObject, MdlChain, NextDevice);
    }
    if (Result)
        InterlockedIncrement(&DevExt->FastIoSuccessCount);
    if (Result)
        InterlockedIncrement(&DevExt->FastIoSuccessCountByIndex[FastIoMdlReadCompleteIndex]);
UnlockReturn:
    UnloadProtectionRelease(&FbDriver->UnloadProtection);
    return Result;
}

BOOLEAN
FbFastIoPrepareMdlWrite(
    _In_ PFILE_OBJECT FileObject,
    _In_ PLARGE_INTEGER FileOffset,
    _In_ ULONG Length,
    _In_ ULONG LockKey,
    _Out_ PMDL *MdlChain,
    _Out_ PIO_STATUS_BLOCK IoStatus,
    _In_ PDEVICE_OBJECT DeviceObject
    )
{
    PFBDEV_EXT DevExt;
    PFBDRIVER FbDriver = GetFbDriver();
    BOOLEAN Result = FALSE;
    PFAST_IO_DISPATCH FastIoDispatch;
    PDEVICE_OBJECT NextDevice;

    UnloadProtectionAcquire(&FbDriver->UnloadProtection);
    if (DeviceObject == FbDriver->CtlDevice)
        goto UnlockReturn;

    KLDbg("FastIo");

    DevExt = (PFBDEV_EXT)DeviceObject->DeviceExtension;
    if (DevExt->Magic != FBDEV_EXT_MAGIC)
        __debugbreak();
    InterlockedIncrement(&DevExt->FastIoCount);
    InterlockedIncrement(&DevExt->FastIoCountByIndex[FastIoPrepareMdlWriteIndex]);
    NextDevice = DevExt->AttachedToDevice;
    FastIoDispatch = NextDevice->DriverObject->FastIoDispatch;
    if (VALID_FAST_IO_DISPATCH_HANDLER(FastIoDispatch, PrepareMdlWrite)) {
        Result = FastIoDispatch->PrepareMdlWrite(FileObject, FileOffset, Length, LockKey, MdlChain,
                                                 IoStatus, NextDevice);
    }
    if (Result)
        InterlockedIncrement(&DevExt->FastIoSuccessCount);
    if (Result)
        InterlockedIncrement(&DevExt->FastIoSuccessCountByIndex[FastIoPrepareMdlWriteIndex]);
UnlockReturn:
    UnloadProtectionRelease(&FbDriver->UnloadProtection);
    return Result;
}

BOOLEAN
FbFastIoMdlWriteComplete(
    _In_ PFILE_OBJECT FileObject,
    _In_ PLARGE_INTEGER FileOffset,
    _In_ PMDL MdlChain,
    _In_ PDEVICE_OBJECT DeviceObject
    )
{
    PFBDEV_EXT DevExt;
    PFBDRIVER FbDriver = GetFbDriver();
    BOOLEAN Result = FALSE;
    PFAST_IO_DISPATCH FastIoDispatch;
    PDEVICE_OBJECT NextDevice;

    UnloadProtectionAcquire(&FbDriver->UnloadProtection);
    if (DeviceObject == FbDriver->CtlDevice)
        goto UnlockReturn;

    KLDbg("FastIo");

    DevExt = (PFBDEV_EXT)DeviceObject->DeviceExtension;
    if (DevExt->Magic != FBDEV_EXT_MAGIC)
        __debugbreak();
    InterlockedIncrement(&DevExt->FastIoCount);
    InterlockedIncrement(&DevExt->FastIoCountByIndex[FastIoMdlWriteCompleteIndex]);
    NextDevice = DevExt->AttachedToDevice;
    FastIoDispatch = NextDevice->DriverObject->FastIoDispatch;
    if (VALID_FAST_IO_DISPATCH_HANDLER(FastIoDispatch, MdlWriteComplete)) {
        Result = FastIoDispatch->MdlWriteComplete(FileObject, FileOffset, MdlChain,
                                                  NextDevice);
    }
    if (Result)
        InterlockedIncrement(&DevExt->FastIoSuccessCount);
    if (Result)
        InterlockedIncrement(&DevExt->FastIoSuccessCountByIndex[FastIoMdlWriteCompleteIndex]);
UnlockReturn:
    UnloadProtectionRelease(&FbDriver->UnloadProtection);
    return Result;
}

NTSTATUS
FbFastIoAcquireForModWrite(
    _In_ PFILE_OBJECT FileObject,
    _In_ PLARGE_INTEGER EndingOffset,
    _Out_ PERESOURCE *ResourceToRelease,
    _In_ PDEVICE_OBJECT DeviceObject
             )
{
    KLErr("Unimplemented");
    return STATUS_NOT_SUPPORTED;
}

NTSTATUS
FbFastIoReleaseForModWrite(
    _In_ PFILE_OBJECT FileObject,
    _In_ PERESOURCE ResourceToRelease,
    _In_ PDEVICE_OBJECT DeviceObject
             )
{
    KLErr("Unimplemented");
    return STATUS_NOT_SUPPORTED;
}

NTSTATUS
FbFastIoAcquireForCcFlush(
    _In_ PFILE_OBJECT FileObject,
    _In_ PDEVICE_OBJECT DeviceObject
             )
{
    KLErr("Unimplemented");
    return STATUS_NOT_SUPPORTED;
}

NTSTATUS
FbFastIoReleaseForCcFlush(
    _In_ PFILE_OBJECT FileObject,
    _In_ PDEVICE_OBJECT DeviceObject
             )
{
    KLErr("Unimplemented");
    return STATUS_NOT_SUPPORTED;
}

BOOLEAN
FbFastIoReadCompressed(
    _In_ PFILE_OBJECT FileObject,
    _In_ PLARGE_INTEGER FileOffset,
    _In_ ULONG Length,
    _In_ ULONG LockKey,
    _Out_ PVOID Buffer,
    _Out_ PMDL *MdlChain,
    _Out_ PIO_STATUS_BLOCK IoStatus,
    _Out_ PCOMPRESSED_DATA_INFO CompressedDataInfo,
    _In_ ULONG CompressedDataInfoLength,
    _In_ PDEVICE_OBJECT DeviceObject
    )
{
    PFBDEV_EXT DevExt;
    PFBDRIVER FbDriver = GetFbDriver();
    BOOLEAN Result = FALSE;
    PFAST_IO_DISPATCH FastIoDispatch;
    PDEVICE_OBJECT NextDevice;

    UnloadProtectionAcquire(&FbDriver->UnloadProtection);
    if (DeviceObject == FbDriver->CtlDevice)
        goto UnlockReturn;

    KLDbg("FastIo");

    DevExt = (PFBDEV_EXT)DeviceObject->DeviceExtension;
    if (DevExt->Magic != FBDEV_EXT_MAGIC)
        __debugbreak();
    InterlockedIncrement(&DevExt->FastIoCount);
    InterlockedIncrement(&DevExt->FastIoCountByIndex[FastIoReadCompressedIndex]);
    NextDevice = DevExt->AttachedToDevice;
    FastIoDispatch = NextDevice->DriverObject->FastIoDispatch;
    if (VALID_FAST_IO_DISPATCH_HANDLER(FastIoDispatch, FastIoReadCompressed)) {
        Result = FastIoDispatch->FastIoReadCompressed(FileObject, FileOffset, Length, LockKey, Buffer, MdlChain,
                                                      IoStatus, CompressedDataInfo, CompressedDataInfoLength,
                                                      NextDevice);
    }
    if (Result)
        InterlockedIncrement(&DevExt->FastIoSuccessCount);
    if (Result)
        InterlockedIncrement(&DevExt->FastIoSuccessCountByIndex[FastIoReadCompressedIndex]);
UnlockReturn:
    UnloadProtectionRelease(&FbDriver->UnloadProtection);
    return Result;
}    

BOOLEAN
FbFastIoWriteCompressed(
    _In_ PFILE_OBJECT FileObject,
    _In_ PLARGE_INTEGER FileOffset,
    _In_ ULONG Length,
    _In_ ULONG LockKey,
    _In_ PVOID Buffer,
    _Out_ PMDL *MdlChain,
    _Out_ PIO_STATUS_BLOCK IoStatus,
    _In_ PCOMPRESSED_DATA_INFO CompressedDataInfo,
    _In_ ULONG CompressedDataInfoLength,
    _In_ PDEVICE_OBJECT DeviceObject
    )
{
    PFBDEV_EXT DevExt;
    PFBDRIVER FbDriver = GetFbDriver();
    BOOLEAN Result = FALSE;
    PFAST_IO_DISPATCH FastIoDispatch;
    PDEVICE_OBJECT NextDevice;

    UnloadProtectionAcquire(&FbDriver->UnloadProtection);
    if (DeviceObject == FbDriver->CtlDevice)
        goto UnlockReturn;

    KLDbg("FastIo");

    DevExt = (PFBDEV_EXT)DeviceObject->DeviceExtension;
    if (DevExt->Magic != FBDEV_EXT_MAGIC)
        __debugbreak();
    InterlockedIncrement(&DevExt->FastIoCount);
    InterlockedIncrement(&DevExt->FastIoCountByIndex[FastIoWriteCompressedIndex]);
    NextDevice = DevExt->AttachedToDevice;
    FastIoDispatch = NextDevice->DriverObject->FastIoDispatch;
    if (VALID_FAST_IO_DISPATCH_HANDLER(FastIoDispatch, FastIoWriteCompressed)) {
        Result = FastIoDispatch->FastIoWriteCompressed(FileObject, FileOffset, Length, LockKey, Buffer, MdlChain,
                                                       IoStatus, CompressedDataInfo, CompressedDataInfoLength,
                                                       NextDevice);
    }
    if (Result)
        InterlockedIncrement(&DevExt->FastIoSuccessCount);
    if (Result)
        InterlockedIncrement(&DevExt->FastIoSuccessCountByIndex[FastIoWriteCompressedIndex]);
UnlockReturn:
    UnloadProtectionRelease(&FbDriver->UnloadProtection);
    return Result;
}    

BOOLEAN
FbFastIoMdlReadCompleteCompressed(
    _In_ PFILE_OBJECT FileObject,
    _In_ PMDL MdlChain,
    _In_ PDEVICE_OBJECT DeviceObject
    )
{
    PFBDEV_EXT DevExt;
    PFBDRIVER FbDriver = GetFbDriver();
    BOOLEAN Result = FALSE;
    PFAST_IO_DISPATCH FastIoDispatch;
    PDEVICE_OBJECT NextDevice;

    UnloadProtectionAcquire(&FbDriver->UnloadProtection);
    if (DeviceObject == FbDriver->CtlDevice)
        goto UnlockReturn;

    KLDbg("FastIo");

    DevExt = (PFBDEV_EXT)DeviceObject->DeviceExtension;
    if (DevExt->Magic != FBDEV_EXT_MAGIC)
        __debugbreak();
    InterlockedIncrement(&DevExt->FastIoCount);
    InterlockedIncrement(&DevExt->FastIoCountByIndex[FastIoMdlReadCompleteCompressedIndex]);
    NextDevice = DevExt->AttachedToDevice;
    FastIoDispatch = NextDevice->DriverObject->FastIoDispatch;
    if (VALID_FAST_IO_DISPATCH_HANDLER(FastIoDispatch, MdlReadCompleteCompressed)) {
        Result = FastIoDispatch->MdlReadCompleteCompressed(FileObject, MdlChain, NextDevice);
    }
    if (Result)
        InterlockedIncrement(&DevExt->FastIoSuccessCount);
    if (Result)
        InterlockedIncrement(&DevExt->FastIoSuccessCountByIndex[FastIoMdlReadCompleteCompressedIndex]);
UnlockReturn:
    UnloadProtectionRelease(&FbDriver->UnloadProtection);
    return Result;
}    

BOOLEAN
FbFastIoMdlWriteCompleteCompressed(
    _In_ PFILE_OBJECT FileObject,
    _In_ PLARGE_INTEGER FileOffset,
    _In_ PMDL MdlChain,
    _In_ PDEVICE_OBJECT DeviceObject
    )
{
    PFBDEV_EXT DevExt;
    PFBDRIVER FbDriver = GetFbDriver();
    BOOLEAN Result = FALSE;
    PFAST_IO_DISPATCH FastIoDispatch;
    PDEVICE_OBJECT NextDevice;

    UnloadProtectionAcquire(&FbDriver->UnloadProtection);
    if (DeviceObject == FbDriver->CtlDevice)
        goto UnlockReturn;

    KLDbg("FastIo");

    DevExt = (PFBDEV_EXT)DeviceObject->DeviceExtension;
    if (DevExt->Magic != FBDEV_EXT_MAGIC)
        __debugbreak();
    InterlockedIncrement(&DevExt->FastIoCount);
    InterlockedIncrement(&DevExt->FastIoCountByIndex[FastIoMdlWriteCompleteCompressedIndex]);
    NextDevice = DevExt->AttachedToDevice;
    FastIoDispatch = NextDevice->DriverObject->FastIoDispatch;
    if (VALID_FAST_IO_DISPATCH_HANDLER(FastIoDispatch, MdlWriteCompleteCompressed)) {
        Result = FastIoDispatch->MdlWriteCompleteCompressed(FileObject, FileOffset, MdlChain, NextDevice);
    }
    if (Result)
        InterlockedIncrement(&DevExt->FastIoSuccessCount);
    if (Result)
        InterlockedIncrement(&DevExt->FastIoSuccessCountByIndex[FastIoMdlWriteCompleteCompressedIndex]);
UnlockReturn:
    UnloadProtectionRelease(&FbDriver->UnloadProtection);
    return Result;
}

BOOLEAN
FbFastIoQueryOpen(
    _Inout_ PIRP Irp,
    _Out_ PFILE_NETWORK_OPEN_INFORMATION NetworkInformation,
    _In_ PDEVICE_OBJECT DeviceObject
    )
{
    PFBDEV_EXT DevExt;
    PFBDRIVER FbDriver = GetFbDriver();
    BOOLEAN Result = FALSE;
    PFAST_IO_DISPATCH FastIoDispatch;
    PDEVICE_OBJECT NextDevice;

    UnloadProtectionAcquire(&FbDriver->UnloadProtection);
    if (DeviceObject == FbDriver->CtlDevice)
        goto UnlockReturn;

    KLDbg("FastIo");

    DevExt = (PFBDEV_EXT)DeviceObject->DeviceExtension;
    if (DevExt->Magic != FBDEV_EXT_MAGIC)
        __debugbreak();
    InterlockedIncrement(&DevExt->FastIoCount);
    InterlockedIncrement(&DevExt->FastIoCountByIndex[FastIoQueryOpenIndex]);
    NextDevice = DevExt->AttachedToDevice;
    FastIoDispatch = NextDevice->DriverObject->FastIoDispatch;
    if (VALID_FAST_IO_DISPATCH_HANDLER(FastIoDispatch, FastIoQueryOpen)) {
        Result = FastIoDispatch->FastIoQueryOpen(Irp, NetworkInformation, NextDevice);
    }
    if (Result)
        InterlockedIncrement(&DevExt->FastIoSuccessCount);
    if (Result)
        InterlockedIncrement(&DevExt->FastIoSuccessCountByIndex[FastIoQueryOpenIndex]);
UnlockReturn:
    UnloadProtectionRelease(&FbDriver->UnloadProtection);
    return Result;
}