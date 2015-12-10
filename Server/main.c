#include "base.h"
#include "log.h"
#include "server.h"
#include "memalloc.h"

#define SVC_NAME L"FBackupServer"
#define SVC_LOG_NAME SVC_NAME L".log"

typedef struct _SVC_CONTEXT {
    SERVICE_STATUS          Status; 
    SERVICE_STATUS_HANDLE   StatusHandle; 
    HANDLE                  hStopEvent;
    HANDLE                  hStoppedEvent;
    volatile LONG           Stopping;
} SVC_CONTEXT, *PSVC_CONTEXT;

SVC_CONTEXT g_SvcCtx;

PSVC_CONTEXT GetSvcContext(VOID)
{
    return &g_SvcCtx;
}

VOID ReportSvcStatus(DWORD dwCurrentState,
                     DWORD dwWin32ExitCode,
                     DWORD dwWaitHint)
{
    static DWORD dwCheckPoint = 1;
    PSVC_CONTEXT SvcContext = GetSvcContext();

    SvcContext->Status.dwCurrentState = dwCurrentState;
    SvcContext->Status.dwWin32ExitCode = dwWin32ExitCode;
    SvcContext->Status.dwWaitHint = dwWaitHint;

    if (dwCurrentState == SERVICE_START_PENDING)
        SvcContext->Status.dwControlsAccepted = 0;
    else
        SvcContext->Status.dwControlsAccepted = SERVICE_ACCEPT_STOP;

    if ((dwCurrentState == SERVICE_RUNNING) ||
        (dwCurrentState == SERVICE_STOPPED))
        SvcContext->Status.dwCheckPoint = 0;
    else
        SvcContext->Status.dwCheckPoint = dwCheckPoint++;

    SetServiceStatus(SvcContext->StatusHandle, &SvcContext->Status);
}

DWORD SvcRun(DWORD argc, WCHAR *argv[])
{
    PSVC_CONTEXT SvcContext = GetSvcContext();
    DWORD Err;

    SvcContext->hStopEvent = CreateEvent(NULL, TRUE, FALSE, NULL);
    if (!SvcContext->hStopEvent) {
        Err = GetLastError();
        LErr("Cant create event Error %d", Err);
        ReportSvcStatus(SERVICE_STOPPED, Err, 0);
        return Err;
    }

    SvcContext->hStoppedEvent = CreateEvent(NULL, TRUE, FALSE, NULL);
    if (!SvcContext->hStoppedEvent) {
        Err = GetLastError();
        LErr("Cant create event Error %d", Err);
        CloseHandle(SvcContext->hStopEvent);
        ReportSvcStatus(SERVICE_STOPPED, Err, 0);
        return Err;
    }

    Err = ServerStart();
    if (Err) {
        LErr("Server start failed Error %d", Err);
        CloseHandle(SvcContext->hStopEvent);
        CloseHandle(SvcContext->hStoppedEvent);
        ReportSvcStatus(SERVICE_STOPPED, Err, 0);
        return Err;
    }

    ReportSvcStatus(SERVICE_RUNNING, NO_ERROR, 0);
    LInf("Starting waiting stop signal loop");
    while(!SvcContext->Stopping) {
        WaitForSingleObject(SvcContext->hStopEvent, INFINITE);
    }

    LInf("Stopping");
    ReportSvcStatus(SERVICE_STOP_PENDING, NO_ERROR, 0);

    ServerStop();

    SetEvent(SvcContext->hStoppedEvent);
    ReportSvcStatus( SERVICE_STOPPED, NO_ERROR, 0);

    LInf("Stopped");

    CloseHandle(SvcContext->hStopEvent);
    CloseHandle(SvcContext->hStoppedEvent);
    return Err;
}

VOID SvcStop()
{
    PSVC_CONTEXT SvcContext = GetSvcContext();

    SvcContext->Stopping = 1;
    SetEvent(SvcContext->hStopEvent);
    WaitForSingleObject(SvcContext->hStoppedEvent, INFINITE);
}

VOID SvcCtrlHandler(DWORD dwCtrl)
{
    PSVC_CONTEXT SvcContext = GetSvcContext();

    LInf("dwCtrl %d", dwCtrl);

    switch(dwCtrl) {  
    case SERVICE_CONTROL_STOP:
        SvcStop();
        return;
    case SERVICE_CONTROL_SHUTDOWN:
        SvcStop();
        return;
    case SERVICE_CONTROL_INTERROGATE: 
        break; 
    default: 
        break;
    } 
    ReportSvcStatus(SvcContext->Status.dwCurrentState, NO_ERROR, 0);
}

VOID SvcMain(DWORD argc, WCHAR *argv[])
{
    PSVC_CONTEXT SvcContext = GetSvcContext();

    SvcContext->StatusHandle = RegisterServiceCtrlHandler(SVC_NAME, SvcCtrlHandler);
    if (!SvcContext->StatusHandle) {
        LErr("Cant create status handle Error %d", GetLastError());
        return;
    }
    SvcContext->Status.dwServiceType = SERVICE_WIN32_OWN_PROCESS;
    SvcContext->Status.dwServiceSpecificExitCode = 0;
    
    ReportSvcStatus(SERVICE_START_PENDING, NO_ERROR, 3000);
    
    SvcRun(argc, argv);
}

SC_HANDLE ScmOpenSCMHandle()
{
    SC_HANDLE hScm = OpenSCManager(NULL, NULL, SC_MANAGER_ALL_ACCESS);

    if (hScm == NULL) {
        LErr("OpenSCManager Error %d", GetLastError());
    }

    return hScm;
}

VOID ScmCloseSCMHandle(SC_HANDLE hScm)
{
    CloseServiceHandle(hScm);
}

DWORD ScmInstallService(SC_HANDLE  hScm, PWCHAR ServiceName, PWCHAR ServiceBinaryPath)
{
    SC_HANDLE hService;
    DWORD Err;

    hService = CreateService(hScm,
        ServiceName,
        ServiceName,
        SERVICE_ALL_ACCESS,
        SERVICE_WIN32_OWN_PROCESS,
        SERVICE_DEMAND_START,
        SERVICE_ERROR_NORMAL,
        ServiceBinaryPath,
        NULL,
        NULL, NULL, NULL, NULL);
    if (hService == NULL)
    {
        Err = GetLastError();
        if (Err == ERROR_SERVICE_EXISTS) {
            LErr("Service already exists Error %d", Err);
        } else {
            LErr("Can't create service Error %d", Err);
        }
        return Err;
    }
    CloseServiceHandle (hService);
    return 0;
}

DWORD ScmDeleteService(SC_HANDLE hScm, PWCHAR ServiceName)
{
    SC_HANDLE hService;
    DWORD Err;

    hService = OpenService(hScm, ServiceName, SERVICE_ALL_ACCESS);
    if (hService == NULL) {
        Err = GetLastError();
        LErr("OpenService Error %d", Err);
        return Err;
    }

    if (!DeleteService(hService)) {
        Err = GetLastError();
        LErr("DeleteService Error %d", Err);
    } else
        Err = 0;

    CloseServiceHandle(hService);
    return Err;
}

DWORD ScmStartService(SC_HANDLE hScm, PWCHAR ServiceName)
{
    SC_HANDLE hService;
    DWORD Err;
    
    hService = OpenService(hScm, ServiceName, SERVICE_ALL_ACCESS);
    if (hService == NULL) {
        Err = GetLastError();
        LErr("OpenService Error %d", Err);
        return Err;
    }

    if (!StartService(hService, 0, NULL))
    {
        Err = GetLastError();
        if (Err == ERROR_SERVICE_ALREADY_RUNNING) {
            LErr("Service already running Err %d", Err);
        } else { 
            LErr("StartService Error %d", Err);
        }
    } else
        Err = 0;

    CloseServiceHandle(hService);
    return Err;
}

DWORD ScmStopService(SC_HANDLE hScm, PWCHAR ServiceName)
{
    SC_HANDLE hService;
    SERVICE_STATUS ServiceStatus;
    DWORD Err;

    hService = OpenService (hScm, ServiceName, SERVICE_ALL_ACCESS);
    if (hService == NULL)
    {
        Err = GetLastError();
        LErr("OpenService Error %d", GetLastError());
        return Err;
    }

    if (!ControlService(hService, SERVICE_CONTROL_STOP, &ServiceStatus))
    {
        Err = GetLastError();
        LErr("ControlService Error %d", Err);
    } else
        Err = 0;

    CloseServiceHandle (hService);
    return Err;
}

BOOL IsCmdEqual(PWCHAR Arg, PWCHAR Cmd)
{
    return (wcsncmp(Arg, Cmd, wcslen(Arg) + 1) == 0);
}

BOOL GetBinaryPath(PWCHAR BinaryFilePath, PWCHAR BinaryPath)
{
    PWCHAR pLastSlash;
    ULONG BinaryPathSize;

    pLastSlash = wcsrchr(BinaryFilePath, L'\\');
    if (!pLastSlash)
        return FALSE;

    BinaryPathSize = (ULONG)((ULONG_PTR)pLastSlash - (ULONG_PTR)BinaryFilePath);
    memcpy(BinaryPath, BinaryFilePath, BinaryPathSize);
    BinaryPath[BinaryPathSize/sizeof(WCHAR)] = L'\0';

    return TRUE;
}

int wmain(int argc, WCHAR* argv[])
{
    DWORD Err;
    SERVICE_TABLE_ENTRY DispatchTable[] = 
    { 
        { SVC_NAME, (LPSERVICE_MAIN_FUNCTION) SvcMain }, 
        { NULL, NULL } 
    }; 
    WCHAR BinaryFilePath[MAX_PATH], BinaryPath[MAX_PATH], LogFilePath[MAX_PATH];

    Err = MemAllocInit();
    if (Err)
        goto terminate;

    if (0 == GetModuleFileName(NULL, BinaryFilePath, MAX_PATH)) {
        Err = GetLastError();
        goto mem_alloc_release;
    }

    if (!GetBinaryPath(BinaryFilePath, BinaryPath)) {
        Err = ERROR_INVALID_PARAMETER;
        goto mem_alloc_release;
    }

    _snwprintf(LogFilePath, RTL_NUMBER_OF(LogFilePath) - 1, L"%ws\\%ws",
               BinaryPath, SVC_LOG_NAME);
    LogFilePath[RTL_NUMBER_OF(LogFilePath) - 1] = '\0';

    Err = GlobalLogInit(LogFilePath, LOG_INF);
    if (Err)
        goto mem_alloc_release;

    LInf("Starting");

    if (argc == 1) {
        if (!StartServiceCtrlDispatcher(DispatchTable))
            Err = GetLastError();
        else
            Err = 0;
    } else {
        if (argc != 2) {
            LErr("Invalid number of args");
            Err = ERROR_INVALID_PARAMETER;
            goto log_release;
        }

        if (IsCmdEqual(argv[1], L"create")) {
            SC_HANDLE hScm;

            hScm = ScmOpenSCMHandle();
            if (!hScm) {
                Err = GetLastError();
                goto log_release;
            }
            Err = ScmInstallService(hScm, SVC_NAME, BinaryFilePath);
            ScmCloseSCMHandle(hScm);
        } else if (IsCmdEqual(argv[1], L"delete")) {
            SC_HANDLE hScm;
            
            hScm = ScmOpenSCMHandle();
            if (!hScm) {
                Err = GetLastError();
                goto log_release;
            }
            Err = ScmDeleteService(hScm, SVC_NAME);
            ScmCloseSCMHandle(hScm);
        } else if (IsCmdEqual(argv[1], L"start")) {
            SC_HANDLE hScm;
            
            hScm = ScmOpenSCMHandle();
            if (!hScm) {
                Err = GetLastError();
                goto log_release;
            }
            Err = ScmStartService(hScm, SVC_NAME);
            ScmCloseSCMHandle(hScm);
        } else if (IsCmdEqual(argv[1], L"stop")) {
            SC_HANDLE hScm;
            
            hScm = ScmOpenSCMHandle();
            if (!hScm) {
                Err = GetLastError();
                goto log_release;
            }
            Err = ScmStopService(hScm, SVC_NAME);
            ScmCloseSCMHandle(hScm);
        } else {
            LErr("Invalid parameter %ws", argv[1]);
            Err = ERROR_INVALID_PARAMETER;
        }
    }
log_release:
    LInf("Exiting Error %d", Err);
    GlobalLogRelease();
mem_alloc_release:
    MemAllocRelease();
terminate:
    TerminateProcess(GetCurrentProcess(), Err);
    return 0;
}

