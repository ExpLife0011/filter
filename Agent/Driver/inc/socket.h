#ifndef __FBACKUP_SOCKET_H__
#define __FBACKUP_SOCKET_H__

#include "inc\base.h"

typedef struct _SOCKET_FACTORY {
    WSK_CLIENT_DISPATCH ClientDispatch;
    WSK_CLIENT_NPI      ClientNpi;
    WSK_REGISTRATION    Registration;
    WSK_PROVIDER_NPI    ProviderNpi;
    LIST_ENTRY          SocketListHead;
    KSPIN_LOCK          Lock;
    volatile LONG       Releasing;
} SOCKET_FACTORY, *PSOCKET_FACTORY;

typedef struct _SOCKET {
    LIST_ENTRY                          ListEntry;
    PWSK_SOCKET                         WskSocket;
    PSOCKET_FACTORY                     SocketFactory;
    KSPIN_LOCK                          Lock;
    volatile LONG                       RefCount;
} SOCKET, *PSOCKET;

NTSTATUS SocketFactoryInit(PSOCKET_FACTORY SocketFactory);

VOID SocketFactoryRelease(PSOCKET_FACTORY SocketFactory);

FORCEINLINE VOID SocketReference(PSOCKET Socket)
{
    if (Socket->RefCount <= 0)
        __debugbreak();
    InterlockedIncrement(&Socket->RefCount);
}

VOID SocketDereference(PSOCKET Socket);

NTSTATUS SocketConnect(PSOCKET_FACTORY SocketFactory, PWCHAR Ip, PWCHAR Port, PSOCKET *pSocket);

NTSTATUS SocketSend(PSOCKET Socket, PVOID Buf, ULONG Size, PULONG pSent);

NTSTATUS SocketReceive(PSOCKET Socket, PVOID Buf, ULONG Size, PULONG pReceived, PBOOLEAN pbDisconnected);

VOID SocketClose(PSOCKET Socket);

#endif