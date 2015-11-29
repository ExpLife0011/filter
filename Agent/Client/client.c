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

DWORD 
CDrvClose(
    IN HANDLE hDevice
    )
{
    DWORD err;

    if (!CloseHandle(hDevice)) {
        err = GetLastError();
        printf("CloseHandle error %d\n", err);
    } else
        err = 0;
    return err;
}

DWORD NTAPI CDrvCtlFltStart()
{
    HANDLE hDevice = NULL;
    DWORD BytesReturned;
    DWORD err;

    hDevice = CDrvOpen();
    if (hDevice == NULL) {
        err = GetLastError();
        return err;
    }

    if( !DeviceIoControl(hDevice,
        IOCTL_FBACKUP_FLT_START,
        NULL, 0,
        NULL, 0,
        &BytesReturned,
        NULL )  )
    {
        err = GetLastError();
        printf( "Error in IOCTL_FBACKUP_FLT_START %d\n", err);
    } else
        err = 0;

    CDrvClose(hDevice);
    return err;
}

DWORD NTAPI CDrvCtlFltStop()
{
    HANDLE hDevice = NULL;
    DWORD BytesReturned;
    DWORD err;

    hDevice = CDrvOpen();
    if (hDevice == NULL) {
        err = GetLastError();
        return err;
    }

    if( !DeviceIoControl(hDevice,
        IOCTL_FBACKUP_FLT_STOP,
        NULL, 0,
        NULL, 0,
        &BytesReturned,
        NULL )  )
    {
        err = GetLastError();
        printf( "Error in IOCTL_FBACKUP_FLT_STOP %d\n", err);
    } else
        err = 0;

    CDrvClose(hDevice);
    return err;
}

DWORD NTAPI CDrvCtlEcho()
{
    HANDLE hDevice = NULL;
    DWORD BytesReturned;
    DWORD err;

    hDevice = CDrvOpen();
    if (hDevice == NULL) {
        err = GetLastError();
        return err;
    }

    if (!DeviceIoControl(hDevice,
        IOCTL_FBACKUP_ECHO,
        NULL, 0,
        NULL, 0,
        &BytesReturned,
        NULL))
    {
        err = GetLastError();
        printf("Error in IOCTL_FBACKUP_ECHO %d\n", err);
    } else
        err = 0;

    CDrvClose(hDevice);
    return err;
}

DWORD NTAPI CDrvCtlBugCheck()
{
    HANDLE hDevice = NULL;
    DWORD BytesReturned;
    DWORD err;

    hDevice = CDrvOpen();
    if (hDevice == NULL) {
        err = GetLastError();
        return err;
    }

    if (!DeviceIoControl(hDevice,
        IOCTL_FBACKUP_BUGCHECK,
        NULL, 0,
        NULL, 0,
        &BytesReturned,
        NULL))
    {
        err = GetLastError();
        printf("Error in IOCTL_FBACKUP_BUGCHECK %d\n", err);
    } else
        err = 0;

    CDrvClose(hDevice);
    return err;
}

DWORD NTAPI CDrvCtlTest()
{
    HANDLE hDevice = NULL;
    DWORD BytesReturned;
    DWORD err;

    hDevice = CDrvOpen();
    if (hDevice == NULL) {
        err = GetLastError();
        return err;
    }

    if (!DeviceIoControl(hDevice,
        IOCTL_FBACKUP_TEST,
        NULL, 0,
        NULL, 0,
        &BytesReturned,
        NULL))
    {
        err = GetLastError();
        printf("Error in IOCTL_FBACKUP_TEST_DRV_LIB %d\n", err);
    } else
        err = 0;

    CDrvClose(hDevice);
    return err;
}

DWORD NTAPI CDrvInstall(WCHAR *BinPath)
{
    SC_HANDLE hScm = NULL;
    DWORD err;
    WCHAR SysDrvPath[MAX_PATH];
    WCHAR SysDir[MAX_PATH];

    if (GetSystemDirectory(SysDir, RTL_NUMBER_OF(SysDir)) <= 0) {
        err = GetLastError();
        printf("GetSystemDirectory failed with err=%d\n", err);
        return err;
    }

    _snwprintf_s(SysDrvPath, RTL_NUMBER_OF(SysDrvPath), _TRUNCATE, L"%ws\\drivers\\%ws", SysDir, FBACKUP_DRV_NAME_EXT_W);
    printf("Driver path %ws\n", BinPath);

    if (!CopyFileW(BinPath, SysDrvPath, FALSE)) {
        err = GetLastError();
        printf("CopyFileW %ws -> %ws err %d\n", BinPath, SysDrvPath, err);
        return err;
    }

    hScm = ScmOpenSCMHandle();
    if (hScm == NULL) {
        err = GetLastError();
        printf("Error OpenSCMHandle %d\n", err);
        return err;
    }
        
    err = ScmInstallDriver(hScm, FBACKUP_DRV_NAME_W, SysDrvPath);
    if (err) {
        if (err == ERROR_SERVICE_EXISTS) {
            printf("Service already exists err %d\n", err);            
        } else {
            printf("Error install driver err %x\n", err);
        }
    }

    ScmCloseSCMHandle(hScm);

    return err;
}

DWORD NTAPI CDrvUninstall()
{
    SC_HANDLE hScm = NULL;
    DWORD err;

    hScm = ScmOpenSCMHandle();
    if (hScm == NULL) {
        err = GetLastError();
        printf("Error OpenSCMHandle err %d\n", err);
        return err;
    }

    err = ScmRemoveDriver(hScm, FBACKUP_DRV_NAME_W);
    if (err)
        printf("Error remove driver err %d\n", err);

    ScmCloseSCMHandle(hScm);

    return err;
}

DWORD NTAPI CDrvStart()
{
    SC_HANDLE hScm = NULL;
    DWORD err;

    hScm = ScmOpenSCMHandle();
    if (hScm == NULL) {
        err = GetLastError();
        printf("Error OpenSCMHandle err %d\n", err);
        return err;
    }

    err = ScmStartDriver(hScm, FBACKUP_DRV_NAME_W);
    if (err)
        printf("Error start driver err %d\n", err);

    ScmCloseSCMHandle(hScm);
    return err;
}

DWORD NTAPI CDrvStop()
{
    SC_HANDLE hScm = NULL;
    DWORD err;

    hScm = ScmOpenSCMHandle();
    if (hScm == NULL) {
        err = GetLastError();
        printf("Error OpenSCMHandle\n");
        return err;
    }

    err = ScmStopDriver(hScm, FBACKUP_DRV_NAME_W);
    if (err)
        printf("Error stop driver %d\n", err);

    ScmCloseSCMHandle(hScm);

    return err;
}

DWORD NTAPI CDrvLoad(WCHAR *BinPath)
{
    DWORD err;
    
    err = CDrvInstall(BinPath);
    if (err)
        return err;

    err = CDrvStart();
    if (err) {
        CDrvUninstall();
        return err;
    }

    return 0;
}

DWORD NTAPI CDrvUnload()
{
    DWORD err;
    
    err = CDrvStop();
    if (err) {
        printf("cant' stop driver service err %d\n", err);
    }
    
    err = CDrvUninstall();
    return err;
}