#include "inc\fastio.h"
#include "inc\klog.h"

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
    KLDbg("FastIo");
    return FALSE;
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
    KLDbg("FastIo");
    return FALSE;
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
    KLDbg("FastIo");
    return FALSE;
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
    KLDbg("FastIo");
    return FALSE;
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
    KLDbg("FastIo");
    return FALSE;
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
    KLDbg("FastIo");
    return FALSE;
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
    KLDbg("FastIo");
    return FALSE;
}

BOOLEAN
FbFastIoUnlockAll(
    _In_ PFILE_OBJECT FileObject,
    _In_ PEPROCESS ProcessId,
    _Out_ PIO_STATUS_BLOCK IoStatus,
    _In_ PDEVICE_OBJECT DeviceObject
    )
{
    KLDbg("FastIo");
    return FALSE;
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
    KLDbg("FastIo");
    return FALSE;
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
    KLDbg("FastIo");
    return FALSE;
}

VOID
FbFastIoAcquireFile(
    _In_ PFILE_OBJECT FileObject
    )
{
    KLDbg("FastIo");
    return;
}

VOID
FbFastIoReleaseFile(
    _In_ PFILE_OBJECT FileObject
    )
{
    KLDbg("FastIo");
    return;
}

VOID
FbFastIoDetachDevice(
    _In_ PDEVICE_OBJECT SourceDevice,
    _In_ PDEVICE_OBJECT TargetDevice
    )
{
    KLDbg("FastIo");
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
    KLDbg("FastIo");
    return FALSE;
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
    KLDbg("FastIo");
    return FALSE;
}

BOOLEAN
FbFastIoMdlReadComplete(
    _In_ PFILE_OBJECT FileObject,
    _In_ PMDL MdlChain,
    _In_ PDEVICE_OBJECT DeviceObject
    )
{
    KLDbg("FastIo");
    return FALSE;
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
    KLDbg("FastIo");
    return FALSE;
}

BOOLEAN
FbFastIoMdlWriteComplete(
    _In_ PFILE_OBJECT FileObject,
    _In_ PLARGE_INTEGER FileOffset,
    _In_ PMDL MdlChain,
    _In_ PDEVICE_OBJECT DeviceObject
    )
{
    KLDbg("FastIo");
    return FALSE;
}

NTSTATUS
FbFastIoAcquireForModWrite(
    _In_ PFILE_OBJECT FileObject,
    _In_ PLARGE_INTEGER EndingOffset,
    _Out_ PERESOURCE *ResourceToRelease,
    _In_ PDEVICE_OBJECT DeviceObject
             )
{
    KLDbg("FastIo");
    return STATUS_NOT_SUPPORTED;
}

NTSTATUS
FbFastIoReleaseForModWrite(
    _In_ PFILE_OBJECT FileObject,
    _In_ PERESOURCE ResourceToRelease,
    _In_ PDEVICE_OBJECT DeviceObject
             )
{
    KLDbg("FastIo");
    return STATUS_NOT_SUPPORTED;
}

NTSTATUS
FbFastIoAcquireForCcFlush(
    _In_ PFILE_OBJECT FileObject,
    _In_ PDEVICE_OBJECT DeviceObject
             )
{
    KLDbg("FastIo");
    return STATUS_NOT_SUPPORTED;
}

NTSTATUS
FbFastIoReleaseForCcFlush(
    _In_ PFILE_OBJECT FileObject,
    _In_ PDEVICE_OBJECT DeviceObject
             )
{
    KLDbg("FastIo");
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
    KLDbg("FastIo");
    return FALSE;
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
    KLDbg("FastIo");
    return FALSE;
}    

BOOLEAN
FbFastIoMdlReadCompleteCompressed(
    _In_ PFILE_OBJECT FileObject,
    _In_ PMDL MdlChain,
    _In_ PDEVICE_OBJECT DeviceObject
    )
{
    KLDbg("FastIo");
    return FALSE;
}    

BOOLEAN
FbFastIoMdlWriteCompleteCompressed(
    _In_ PFILE_OBJECT FileObject,
    _In_ PLARGE_INTEGER FileOffset,
    _In_ PMDL MdlChain,
    _In_ PDEVICE_OBJECT DeviceObject
    )
{
    KLDbg("FastIo");
    return FALSE;
}

BOOLEAN
FbFastIoQueryOpen(
    _Inout_ PIRP Irp,
    _Out_ PFILE_NETWORK_OPEN_INFORMATION NetworkInformation,
    _In_ PDEVICE_OBJECT DeviceObject
    )
{
    KLDbg("FastIo");
    return FALSE;
}