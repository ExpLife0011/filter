#include "time.h"
#include "log.h"

DWORD FbTimeStart(PFBTIME Time)
{
    DWORD Err;

    memset(Time, 0, sizeof(*Time));
    if (!QueryPerformanceCounter(&Time->Start)) {
        Err = GetLastError();
        LErr("QueryPerformanceCounter failed Error %d", Err);
        return Err;
    }

    if (!QueryPerformanceFrequency(&Time->Freq)) {
        Err = GetLastError();
        LErr("QueryPerformanceFrequency failed Error %d", Err);
        return Err;
    }

    if (Time->Freq.QuadPart == 0) {
        LErr("Time->Freq.QuadPart == 0");
        return FB_E_INVAL;
    }

    return 0;
}

DWORD FbTimeStop(PFBTIME Time)
{
    DWORD Err;

    if (Time->Freq.QuadPart == 0)
        return FB_E_INVAL;

    if (!QueryPerformanceCounter(&Time->Stop)) {
        Err = GetLastError();
        LErr("QueryPerformanceCounter failed Error %d", Err);
        return Err;
    }

    if (Time->Stop.QuadPart < Time->Start.QuadPart) {
        LErr("Time->Stop.QuadPart < Time->Start.QuadPart");
        return FB_E_INVAL;
    }

    Time->DeltaNs = (1000000000ULL*(Time->Stop.QuadPart - Time->Start.QuadPart))/Time->Freq.QuadPart;
    return 0;
}