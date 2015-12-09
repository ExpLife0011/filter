#ifndef __FBACKUP_CTL_CLIENT_H__
#define __FBACKUP_CTL_CLIENT_H__

#include "base.h"
#include "..\driver\h\ioctl.h"

DWORD NTAPI CDrvCtlBugCheck();
DWORD NTAPI CDrvCtlTest();
DWORD NTAPI CDrvCtlEcho();

DWORD NTAPI CDrvLoad(WCHAR *BinPath);
DWORD NTAPI CDrvUnload();

DWORD NTAPI CDrvCtlFltStart();
DWORD NTAPI CDrvCtlFltStop();

#endif