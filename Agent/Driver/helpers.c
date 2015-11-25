#include "inc\helpers.h"
#include "inc\mtags.h"
#include "inc\klog.h"

NTSTATUS GetObjectName(PVOID Object, PUNICODE_STRING *pObjectName)
{
    POBJECT_NAME_INFORMATION ObjectNameInfo;
    NTSTATUS Status;
    ULONG ReturnLength;

    ObjectNameInfo = NULL;
    Status = ObQueryNameString(Object, ObjectNameInfo, 0, &ReturnLength);
    if (!NT_SUCCESS(Status) && (Status != STATUS_INFO_LENGTH_MISMATCH)) {
        KLErr("ObQueryNameString Object %p Status 0x%x", Object, Status);
        return Status;
    }

    ObjectNameInfo = NpAlloc(ReturnLength, MTAG_GEN);
    if (!ObjectNameInfo)
        return STATUS_INSUFFICIENT_RESOURCES;

    Status = ObQueryNameString(Object, ObjectNameInfo, ReturnLength, &ReturnLength);
    if (!NT_SUCCESS(Status)) {
        KLErr("ObQueryNameString Object %p Status 0x%x", Object, Status);
        NpFree(ObjectNameInfo, MTAG_GEN);
        return Status;
    }

    *pObjectName = &ObjectNameInfo->Name;
    return Status;
}