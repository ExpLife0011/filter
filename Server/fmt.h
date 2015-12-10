#ifndef __FBACKUP_SERVER_FMT_H__
#define __FBACKUP_SERVER_FMT_H__

#include "base.h"

LONG FmtMsg2(PCHAR *pBuff, ULONG *pLeft, PCHAR Fmt, va_list Args);
LONG FmtMsg(PCHAR *pBuff, ULONG *pLeft, PCHAR Fmt, ...);
PCHAR FmtTruncatePath(PCHAR FileName);

#endif