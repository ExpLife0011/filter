#include <stdio.h>
#include "scmload.h"

SC_HANDLE ScmOpenSCMHandle()
{
    SC_HANDLE hScm = OpenSCManager(NULL, NULL, SC_MANAGER_ALL_ACCESS);

    if (hScm == NULL) {
        printf("Error OpenSCManager %d\n", GetLastError());
    }

    return hScm;
}

VOID ScmCloseSCMHandle(SC_HANDLE hScm)
{
    CloseServiceHandle(hScm);
}

DWORD ScmInstallDriver(SC_HANDLE  hScm, LPCTSTR DriverName, LPCTSTR DriverExec)
{
    SC_HANDLE hService;
    DWORD err;

    hService = CreateService(hScm,
        DriverName,
        DriverName,
        SERVICE_ALL_ACCESS,
        SERVICE_KERNEL_DRIVER,
        SERVICE_DEMAND_START,
        SERVICE_ERROR_NORMAL,
        DriverExec,
        NULL,
        NULL, NULL, NULL, NULL);
    if (hService == NULL)
    {
        err = GetLastError();
        if (err == ERROR_SERVICE_EXISTS) {
            printf("Service already exists err %d\n", err);
        } else {
            printf("Error can't create service %d\n", err);
        }
        return err;
    }
    CloseServiceHandle (hService);
    return 0;
}

DWORD ScmRemoveDriver(SC_HANDLE hScm, LPCTSTR DriverName)
{
    SC_HANDLE hService;
    DWORD err;

    hService = OpenService(hScm, DriverName, SERVICE_ALL_ACCESS);
    if (hService == NULL) {
        err = GetLastError();
        printf("OpenService error %d\n", err);
        return err;
    }

    if (!DeleteService(hService)) {
        err = GetLastError();
        printf("DeleteService error %d\n", err);
    } else
        err = 0;

    CloseServiceHandle(hService);
    return err;
}

DWORD ScmStartDriver(SC_HANDLE hScm, LPCTSTR DriverName)
{
    SC_HANDLE hService;
    DWORD err;
    
    hService = OpenService(hScm, DriverName, SERVICE_ALL_ACCESS);
    if (hService == NULL) {
        err = GetLastError();
        printf("OpenService error %d\n", err);
        return err;
    }

    if (!StartService(hService, 0, NULL))
    {
        err = GetLastError();
        if (err == ERROR_SERVICE_ALREADY_RUNNING) {
            printf("Service already running err %d\n", err);
        } else { 
            printf("StartService error %d\n", err);
        }
    } else
        err = 0;

    CloseServiceHandle(hService);
    return err;
}

DWORD ScmStopDriver(SC_HANDLE hScm, LPCTSTR DriverName)
{
    SC_HANDLE hService;
    SERVICE_STATUS ServiceStatus;
    DWORD err;

    hService = OpenService (hScm, DriverName, SERVICE_ALL_ACCESS);
    if (hService == NULL)
    {
        err = GetLastError();
        printf("OpenService error %d\n", GetLastError());
        return err;
    }

    if (!ControlService(hService, SERVICE_CONTROL_STOP, &ServiceStatus))
    {
        err = GetLastError();
        printf("ControlService error %d\n", err);
    } else
        err = 0;

    CloseServiceHandle (hService);
    return err;
}

