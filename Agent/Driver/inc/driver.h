#ifndef __FBACKUP_DRIVER_H__
#define __FBACKUP_DRIVER_H__

#include "inc/base.h"

NTSTATUS DriverEntry(PDRIVER_OBJECT  DriverObject, PUNICODE_STRING RegistryPath);

#endif