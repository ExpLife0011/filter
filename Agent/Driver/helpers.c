#include "inc\helpers.h"
#include "inc\mtags.h"
#include "inc\klog.h"
#include "inc\ntapiex.h"

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

NTSTATUS GetObjectByName(PUNICODE_STRING ObjName, POBJECT_TYPE ObjectType, PVOID *pObject)
{
    PVOID Object;
    NTSTATUS Status;
    OBJECT_ATTRIBUTES ObjAttrs;
    HANDLE ObjectHandle;

    InitializeObjectAttributes(&ObjAttrs, ObjName, OBJ_CASE_INSENSITIVE|OBJ_KERNEL_HANDLE, NULL, NULL);

    Status = ObOpenObjectByName(&ObjAttrs, ObjectType, KernelMode, NULL, 0, NULL, &ObjectHandle);
    if (!NT_SUCCESS(Status)) {
        KLErr("Can't open object by name Status 0x%x", Status);
        return Status;
    }

    Status = ObReferenceObjectByHandle(ObjectHandle, 0, ObjectType, KernelMode, &Object, NULL);
    if (!NT_SUCCESS(Status)) {
        KLErr("Can't reference object by handle %p Status 0x%x", ObjectHandle, Status);
        ZwClose(ObjectHandle);
        return Status;
    }
    ZwClose(ObjectHandle);
    *pObject = Object;
    return STATUS_SUCCESS;
}
