#ifndef __FBACKUP_CTL_SCM_LOAD_H__
#define __FBACKUP_CTL_SCM_LOAD_H__

#include <Windows.h>

SC_HANDLE ScmOpenSCMHandle();

VOID ScmCloseSCMHandle(SC_HANDLE hScm);

DWORD ScmInstallDriver(SC_HANDLE hScm, LPCTSTR DriverName, LPCTSTR DriverExec);

DWORD ScmRemoveDriver(SC_HANDLE hScm, LPCTSTR DriverName);

DWORD ScmStartDriver(SC_HANDLE hScm, LPCTSTR DriverName);

DWORD ScmStopDriver(SC_HANDLE hScm, LPCTSTR DriverName);

#endif