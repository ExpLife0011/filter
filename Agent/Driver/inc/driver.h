#ifndef __FBACKUP_DRIVER_H__
#define __FBACKUP_DRIVER_H__

#include "inc\base.h"
#include "inc\unload_protection.h"

NTSTATUS DriverEntry(PDRIVER_OBJECT  DriverObject, PUNICODE_STRING RegistryPath);

#define FBDEV_EXT_MAGIC 0xCBDACBDA

enum {
    FastIoCheckIfPossibleIndex,
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
    ULONG Magic;
    PDEVICE_OBJECT DeviceObject;
    UNICODE_STRING SymLinkName;
    BOOLEAN SymLinkCreated;
    PDEVICE_OBJECT TargetDevice;
    PDEVICE_OBJECT AttachedToDevice;
    volatile LONG   IrpMjCount[IRP_MJ_MAXIMUM_FUNCTION+1];
    volatile LONG   IrpCount;
    volatile LONG   FastIoCountByIndex[FastIoMaxIndex];
    volatile LONG   FastIoSuccessCountByIndex[FastIoMaxIndex];
    volatile LONG   FastIoCount;
    volatile LONG   FastIoSuccessCount;
} FBDEV_EXT, *PFBDEV_EXT;

typedef struct _FBDRIVER {
    PDRIVER_OBJECT Driver;
    PDEVICE_OBJECT CtlDevice;
    PDRIVER_OBJECT NtfsDriver;
    UNLOAD_PROTECTION UnloadProtection;
    KGUARDED_MUTEX Lock;
} FBDRIVER, *PFBDRIVER;

PFBDRIVER GetFbDriver(VOID);

#endif