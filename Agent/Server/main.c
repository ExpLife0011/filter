#include "base.h"
#include "log.h"

#define SVCNAME L"FBackupServer"

typedef struct _SVC_CONTEXT {
    SERVICE_STATUS          Status; 
    SERVICE_STATUS_HANDLE   StatusHandle; 
    HANDLE                  hStopEvent;
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

    // Report the status of the service to the SCM.
    SetServiceStatus(SvcContext->StatusHandle, &SvcContext->Status);
}

VOID SvcRun(DWORD argc, WCHAR *argv[])
{
    PSVC_CONTEXT SvcContext = GetSvcContext();

    SvcContext->hStopEvent = CreateEvent(NULL, TRUE, FALSE, NULL);
    if (!SvcContext->hStopEvent) {
        LErr("Cant create event Error %d", GetLastError());
        ReportSvcStatus( SERVICE_STOPPED, NO_ERROR, 0);
        return;
    }

    ReportSvcStatus( SERVICE_RUNNING, NO_ERROR, 0);
    LInf("Starting stop waiting");
    while(!SvcContext->Stopping) {
        WaitForSingleObject(SvcContext->hStopEvent, INFINITE);
    }
    LInf("Stopping");
    ReportSvcStatus( SERVICE_STOPPED, NO_ERROR, 0);
}

VOID SvcCtrlHandler( DWORD dwCtrl )
{
    PSVC_CONTEXT SvcContext = GetSvcContext();

    LInf("dwCtrl %d", dwCtrl);

    switch(dwCtrl) {  
    case SERVICE_CONTROL_STOP:
        ReportSvcStatus(SERVICE_STOP_PENDING, NO_ERROR, 0);
        SetEvent(SvcContext->hStopEvent);
        ReportSvcStatus(SERVICE_STOPPED, NO_ERROR, 0);
        return;
    case SERVICE_CONTROL_SHUTDOWN:
        ReportSvcStatus(SERVICE_STOP_PENDING, NO_ERROR, 0);
        SetEvent(SvcContext->hStopEvent);
        ReportSvcStatus(SERVICE_STOPPED, NO_ERROR, 0);
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

    SvcContext->StatusHandle = RegisterServiceCtrlHandler(SVCNAME, SvcCtrlHandler);
    if (!SvcContext->StatusHandle) {
        LErr("Cant create status handle Err %d", GetLastError());
        return;
    }
    SvcContext->Status.dwServiceType = SERVICE_WIN32_OWN_PROCESS;
    SvcContext->Status.dwServiceSpecificExitCode = 0;
    
    ReportSvcStatus(SERVICE_START_PENDING, NO_ERROR, 3000);
    
    SvcRun(argc, argv);
}

int wmain(int argc, WCHAR* argv[])
{
    DWORD Err;
    SERVICE_TABLE_ENTRY DispatchTable[] = 
    { 
        { SVCNAME, (LPSERVICE_MAIN_FUNCTION) SvcMain }, 
        { NULL, NULL } 
    }; 

    Err = GlobalLogInit(L"C:\\FbackupServer.log");
    if (Err)
        goto terminate;

    LInf("Starting");

    if (!StartServiceCtrlDispatcher(DispatchTable))
        Err = GetLastError();
    else
        Err = 0;

    LInf("Stopping Err %d", Err);
    GlobalLogRelease();
terminate:
    TerminateProcess(GetCurrentProcess(), Err);
    return 0;
}

