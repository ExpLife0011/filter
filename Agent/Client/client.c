#include <Windows.h>
#include <stdio.h>
#include "scmload.h"
#include "client.h"

HANDLE CDrvOpen()
{
    HANDLE hDevice = CreateFile(FBACKUP_DRV_WIN32_DEVICE_NAME_W,
        GENERIC_READ | GENERIC_WRITE,
        FILE_SHARE_READ | FILE_SHARE_WRITE,
        NULL,
        OPEN_EXISTING,
        FILE_ATTRIBUTE_NORMAL,
        NULL );

    if (hDevice == INVALID_HANDLE_VALUE) {
        printf("ERROR: can not access driver %ws, error %d\n", FBACKUP_DRV_WIN32_DEVICE_NAME_W, GetLastError());
        return NULL;
    }

    return hDevice;
}

BOOL 
CDrvClose(
    IN HANDLE hDevice
    )
{
    return CloseHandle(hDevice);
}

DWORD NTAPI CDrvCtlFltStart()
{
    HANDLE hDevice = NULL;
    DWORD BytesReturned;
    DWORD Result = -1;

    hDevice = CDrvOpen();
    if (hDevice == NULL) {
        return -1;
    }

    if( !DeviceIoControl(hDevice,
        IOCTL_FBACKUP_FLT_START,
        NULL, 0,
        NULL, 0,
        &BytesReturned,
        NULL )  )
    {
        printf( "Error in IOCTL_FBACKUP_FLT_START %d\n", GetLastError());
        Result = -1;
    } else {
        Result = 0;
    }

    CDrvClose(hDevice);
    return Result;
}

DWORD NTAPI CDrvCtlFltStop()
{
    HANDLE hDevice = NULL;
    DWORD BytesReturned;
    DWORD Result = -1;

    hDevice = CDrvOpen();
    if (hDevice == NULL) {
        return -1;
    }

    if( !DeviceIoControl(hDevice,
        IOCTL_FBACKUP_FLT_STOP,
        NULL, 0,
        NULL, 0,
        &BytesReturned,
        NULL )  )
    {
        printf( "Error in IOCTL_FBACKUP_FLT_STOP %d\n", GetLastError());
        Result = -1;
    } else {
        Result = 0;
    }

    CDrvClose(hDevice);
    return Result;
}

DWORD NTAPI CDrvCtlEcho()
{
    HANDLE hDevice = NULL;
    DWORD BytesReturned;
    DWORD Result = -1;

    hDevice = CDrvOpen();
    if (hDevice == NULL) {
        return -1;
    }

    if (!DeviceIoControl(hDevice,
        IOCTL_FBACKUP_ECHO,
        NULL, 0,
        NULL, 0,
        &BytesReturned,
        NULL))
    {
        printf("Error in IOCTL_FBACKUP_ECHO %d\n", GetLastError());
        Result = -1;
    }
    else {
        Result = 0;
    }

    CDrvClose(hDevice);
    return Result;
}

DWORD NTAPI CDrvCtlBugCheck()
{
    HANDLE hDevice = NULL;
    DWORD BytesReturned;
    DWORD Result = -1;

    hDevice = CDrvOpen();
    if (hDevice == NULL) {
        return -1;
    }

    if (!DeviceIoControl(hDevice,
        IOCTL_FBACKUP_BUGCHECK,
        NULL, 0,
        NULL, 0,
        &BytesReturned,
        NULL))
    {
        printf("Error in IOCTL_FBACKUP_BUGCHECK %d\n", GetLastError());
        Result = -1;
    }
    else {
        Result = 0;
    }

    CDrvClose(hDevice);
    return Result;
}

DWORD NTAPI CDrvCtlTest()
{
    HANDLE hDevice = NULL;
    DWORD BytesReturned;
    DWORD Result = -1;

    hDevice = CDrvOpen();
    if (hDevice == NULL) {
        return -1;
    }

    if (!DeviceIoControl(hDevice,
        IOCTL_FBACKUP_TEST,
        NULL, 0,
        NULL, 0,
        &BytesReturned,
        NULL))
    {
        printf("Error in IOCTL_FBACKUP_TEST_DRV_LIB %d\n", GetLastError());
        Result = -1;
    }
    else {
        Result = 0;
    }

    CDrvClose(hDevice);
    return Result;
}

DWORD NTAPI CDrvInstall(WCHAR *BinPath)
{
    SC_HANDLE hscm = NULL;
    DWORD err;
    WCHAR SysDrvPath[MAX_PATH];
    WCHAR SysDir[MAX_PATH];

    if (GetSystemDirectory(SysDir, RTL_NUMBER_OF(SysDir)) <= 0) {
        printf("GetSystemDirectory failed with err=%d\n", GetLastError());
        return -1;
    }

    _snwprintf_s(SysDrvPath, RTL_NUMBER_OF(SysDrvPath), _TRUNCATE, L"%ws\\drivers\\%ws", SysDir, FBACKUP_DRV_NAME_EXT_W);
    printf("Driver path %ws\n", BinPath);

    if (!CopyFileW(BinPath, SysDrvPath, FALSE)) {
        err = GetLastError();
        printf("CopyFileW %ws -> %ws err %d\n", BinPath, SysDrvPath, err);
        return -1;
    }

    hscm = ScmOpenSCMHandle();
    if (hscm == INVALID_HANDLE_VALUE) {
        printf("Error OpenSCMHandle\n");
        return -1;
    }
        
    if (!ScmInstallDriver(hscm, FBACKUP_DRV_NAME_W, SysDrvPath)) {
        err = GetLastError();
        if (err == ERROR_SERVICE_EXISTS) {
            goto cleanup;
        }
        printf("Error install driver err %x\n", err);
        return -1;
    }

cleanup:
    if (hscm != NULL) {
        ScmCloseSCMHandle(hscm);
    }

    return 0;
}

DWORD NTAPI CDrvUninstall()
{
    SC_HANDLE hscm = NULL;
    hscm = ScmOpenSCMHandle();
    if (hscm == INVALID_HANDLE_VALUE) {
        printf("Error OpenSCMHandle\n");
        return -1;
    }

    if (!ScmRemoveDriver(hscm, FBACKUP_DRV_NAME_W)) {
        printf("Error remove driver\n");
        return -1;
    }

    if (hscm != NULL) {
        ScmCloseSCMHandle(hscm);
    }
    return 0;
}

DWORD NTAPI CDrvStart()
{
    SC_HANDLE hscm = NULL;
    hscm = ScmOpenSCMHandle();
    if (hscm == INVALID_HANDLE_VALUE) {
        printf("Error OpenSCMHandle\n");
        return -1;
    }

    if (!ScmStartDriver(hscm, FBACKUP_DRV_NAME_W)) {
        printf("Error start driver\n");
        return -1;
    }

    if (hscm != NULL) {
        ScmCloseSCMHandle(hscm);
    }
    return 0;
}

DWORD NTAPI CDrvStop()
{
    SC_HANDLE hscm = NULL;
    hscm = ScmOpenSCMHandle();
    if (hscm == INVALID_HANDLE_VALUE) {
        printf("Error OpenSCMHandle\n");
        return -1;
    }

    if (!ScmStopDriver(hscm, FBACKUP_DRV_NAME_W)) {
        printf("Error stop driver\n");
        return -1;
    }

    if (hscm != NULL) {
        ScmCloseSCMHandle(hscm);
    }

    return 0;
}

DWORD NTAPI CDrvLoad(WCHAR *BinPath)
{
    if (CDrvInstall(BinPath) < 0) {
        goto out;
    }

    if (CDrvStart() < 0) {
        goto rem_drv;
    }

    return 0;
rem_drv:
    CDrvUninstall();
out:
    return -1;
}

DWORD NTAPI CDrvUnload()
{
    CDrvStop();
    CDrvUninstall();
    return 0;
}