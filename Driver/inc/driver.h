#ifndef __FBACKUP_DRIVER_H__
#define __FBACKUP_DRIVER_H__

#include "inc\base.h"
#include "inc\unload_protection.h"
#include "inc\worker.h"
#include "inc\socket.h"

NTSTATUS DriverEntry(PDRIVER_OBJECT  DriverObject, PUNICODE_STRING RegistryPath);

#define FBDEV_EXT_MAGIC         0xCBDECBDE
#define FBDEV_IRP_CONTEXT_MAGIC 0xBCDEBCDE

enum {
    FastIoCheckIfPossibleIndex = 0,
    FastIoReadIndex,
    FastIoWriteIndex,
    FastIoQueryBasicInfoIndex,
    FastIoQueryStandardInfoIndex,
    FastIoLockIndex,
    FastIoUnlockSingleIndex,
    FastIoUnlockAllIndex,
    FastIoUnlockAllByKeyIndex,
    FastIoDeviceControlIndex,
    FastIoAcquireFileIndex,
    FastIoReleaseFileIndex,
    FastIoDetachDeviceIndex,
    FastIoQueryNetworkOpenInfoIndex,
    FastIoMdlReadIndex,
    FastIoMdlReadCompleteIndex,
    FastIoPrepareMdlWriteIndex,
    FastIoMdlWriteCompleteIndex,
    FastIoAcquireForModWriteIndex,
    FastIoReleaseForModWriteIndex,
    FastIoAcquireForCcFlushIndex,
    FastIoReleaseForCcFlushIndex,
    FastIoReadCompressedIndex,
    FastIoWriteCompressedIndex,
    FastIoMdlReadCompleteCompressedIndex,
    FastIoMdlWriteCompleteCompressedIndex,
    FastIoQueryOpenIndex,
    FastIoMaxIndex
};

typedef struct _FBDEV_EXT
{
    ULONG           Magic;
    PDEVICE_OBJECT  DeviceObject;
    UNICODE_STRING  SymLinkName;
    BOOLEAN         SymLinkCreated;
    PDEVICE_OBJECT  TargetDevice;
    PDEVICE_OBJECT  AttachedToDevice;
    volatile LONG   RefCount;
    KEVENT          ReleasedEvent;
    volatile LONG   IrpMjCount[IRP_MJ_MAXIMUM_FUNCTION+1];
    volatile LONG   IrpCount;
    volatile LONG   IrpCompletionCount;
    volatile LONG   FastIoCountByIndex[FastIoMaxIndex];
    volatile LONG   FastIoSuccessCountByIndex[FastIoMaxIndex];
    volatile LONG   FastIoCount;
    volatile LONG   FastIoSuccessCount;
} FBDEV_EXT, *PFBDEV_EXT;

typedef struct _FBDEV_IRP_CONTEXT {
    ULONG Magic;
    PIRP Irp;
    PFBDEV_EXT DevExt;
} FBDEV_IRP_CONTEXT, *PFBDEV_IRP_CONTEXT;

typedef struct _FBDRIVER {
    RTL_OSVERSIONINFOEXW    OsVersion;
    PDRIVER_OBJECT          Driver;
    PDEVICE_OBJECT          CtlDevice;
    PDRIVER_OBJECT          NtfsDriver;
    UNLOAD_PROTECTION       UnloadProtection;
    WORKER                  MainWorker;
    SOCKET_FACTORY          SocketFactory;
} FBDRIVER, *PFBDRIVER;

PFBDRIVER GetFbDriver(VOID);

VOID FbFltDetachDevice(PDEVICE_OBJECT SourceDevice, PDEVICE_OBJECT TargetDevice);
VOID FbDriverDeviceDelete(PDEVICE_OBJECT Device);

FORCEINLINE PFBDEV_EXT FbDevExtByDevice(PDEVICE_OBJECT DeviceObject)
{
    PFBDEV_EXT DevExt = (PFBDEV_EXT)DeviceObject->DeviceExtension;

    if (DevExt->Magic != FBDEV_EXT_MAGIC) {
        __debugbreak();
        return NULL;
    }

    return DevExt;
}

FORCEINLINE VOID FbDevExtReference(PFBDEV_EXT DevExt)
{
    InterlockedIncrement(&DevExt->RefCount);
}

FORCEINLINE VOID FbDevExtDereference(PFBDEV_EXT DevExt)
{
    if (0 == InterlockedDecrement(&DevExt->RefCount)) {
        KeSetEvent(&DevExt->ReleasedEvent, 0, FALSE);
    }
}

FORCEINLINE VOID FbDevExtWaitRelease(PFBDEV_EXT DevExt)
{
    KeWaitForSingleObject(&DevExt->ReleasedEvent, Executive, KernelMode, FALSE, NULL);
}

#endif