#ifndef __FBACKUP_CTL_SCM_LOAD_H__
#define __FBACKUP_CTL_SCM_LOAD_H__

#include <Windows.h>

SC_HANDLE ScmOpenSCMHandle();

VOID ScmCloseSCMHandle(SC_HANDLE hscm);

BOOL ScmInstallDriver( SC_HANDLE  scm, LPCTSTR DriverName, LPCTSTR driverExec );

BOOL ScmRemoveDriver(SC_HANDLE scm, LPCTSTR DriverName);

BOOL ScmStartDriver(SC_HANDLE  scm, LPCTSTR DriverName);

BOOL ScmStopDriver(SC_HANDLE  scm, LPCTSTR DriverName);

#endif