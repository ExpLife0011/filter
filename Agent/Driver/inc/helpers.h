#ifndef __FBACKUP_HELPERS_H__
#define __FBACKUP_HELPERS_H__

#include "inc\base.h"

NTSTATUS GetObjectName(PVOID Object, PUNICODE_STRING *pObjectName);
NTSTATUS GetObjectByName(PUNICODE_STRING ObjName, POBJECT_TYPE ObjectType, PVOID *pObject);

#endif