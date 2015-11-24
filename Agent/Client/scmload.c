#include <stdio.h>
#include "scmload.h"

SC_HANDLE ScmOpenSCMHandle()
{
    SC_HANDLE hscm = OpenSCManager(NULL,NULL,SC_MANAGER_ALL_ACCESS);

    if (hscm == NULL) {
        printf("Error OpenSCManager %d\n", GetLastError());
    }

    return hscm;
}

VOID ScmCloseSCMHandle(SC_HANDLE hscm)
{
    CloseServiceHandle(hscm);
}

BOOL ScmInstallDriver( SC_HANDLE  scm, LPCTSTR DriverName, LPCTSTR driverExec )
{
    SC_HANDLE Service =
        CreateService ( scm,
        DriverName,
        DriverName,
        SERVICE_ALL_ACCESS,
        SERVICE_KERNEL_DRIVER,
        SERVICE_DEMAND_START,
        SERVICE_ERROR_NORMAL,
        driverExec,
        NULL,
        NULL, NULL, NULL, NULL);

    if (Service == NULL)
    {
        DWORD err = GetLastError();
        if (err == ERROR_SERVICE_EXISTS) {
        } else  printf("Error can't create service %d\n", err);

        return FALSE;
    }
    CloseServiceHandle (Service);
    return TRUE;
}

BOOL ScmRemoveDriver(SC_HANDLE scm, LPCTSTR DriverName)
{
    SC_HANDLE Service =
        OpenService (scm, DriverName, SERVICE_ALL_ACCESS);
    if (Service == NULL) {
        printf("OpenService error %d\n", GetLastError());
        return FALSE;
    }

    BOOL ret = DeleteService (Service);
    if (!ret) {
        printf("DeleteService error %d\n", GetLastError());
    }

    CloseServiceHandle (Service);
    return ret;
}

BOOL ScmStartDriver(SC_HANDLE  scm, LPCTSTR DriverName)
{
    SC_HANDLE Service = OpenService(scm, DriverName, SERVICE_ALL_ACCESS);

    if (Service == NULL) { 
        printf("OpenService error %d\n", GetLastError());
        return FALSE;
    }

    BOOL ret =
        StartService( Service, 0, NULL);

    if (!ret)
    {
        DWORD err = GetLastError();
        if (err == ERROR_SERVICE_ALREADY_RUNNING)
            ret = TRUE;
        else { 
            printf("StartService error %d\n", err);
        }
    }

    CloseServiceHandle (Service);
    return ret;
}

BOOL ScmStopDriver(SC_HANDLE  scm, LPCTSTR DriverName)
{
    SC_HANDLE Service = OpenService (scm, DriverName, SERVICE_ALL_ACCESS );

    if (Service == NULL)
    {
        DWORD err = GetLastError();
        printf("OpenService error %d\n", GetLastError());
        return FALSE;
    }

    SERVICE_STATUS serviceStatus;
    BOOL ret = ControlService(Service, SERVICE_CONTROL_STOP, &serviceStatus);
    if (!ret)
    {
        DWORD err = GetLastError();
        printf("ControlService error %d\n", GetLastError());
    }

    CloseServiceHandle (Service);
    return ret;
}

