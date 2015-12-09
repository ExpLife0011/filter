#include "inc\tests.h"
#include "inc\map.h"
#include "inc\worker.h"
#include "inc\socket.h"
#include "inc\klog.h"
#include "inc\helpers.h"

#define TESTS_TAG 'tseT'

NTSTATUS SocketTest(PVOID Context)
{
    PSOCKET_FACTORY SocketFactory = NULL;
    PSOCKET Socket;
    NTSTATUS Status;
    CHAR HttpResponse[4];
    CHAR HttpRequest[] = "GET / HTTP/1.1\nHost: rbc.ru\nUser-Agent: Mozilla/5.0 (Windows NT 6.3; WOW64)\nAccept: */*\n\n";
    ULONG Sent, Received;
    BOOLEAN Disconnected;

    SocketFactory = NpAlloc(sizeof(*SocketFactory), TESTS_TAG);
    if (!SocketFactory)
        return STATUS_INSUFFICIENT_RESOURCES;

    Status = SocketFactoryInit(SocketFactory);
    if (!NT_SUCCESS(Status)) {
        KLErr("SocketFactoryInit failed Status 0x%x", Status);
        goto socket_factory_free;
    }

    Status = SocketConnect(SocketFactory, L"rbc.ru", L"80", &Socket);
    if (!NT_SUCCESS(Status)) {
        KLErr("SocketConnect failed Status 0x%x", Status);
        goto socket_factory_release;
    }

    Status = SocketSend(Socket, HttpRequest, sizeof(HttpRequest), &Sent);
    if (!NT_SUCCESS(Status)) {
        KLErr("SocketSend failed Status 0x%x", Status);
        goto socket_close;
    }

    if (Sent != sizeof(HttpRequest)) {
        Status = STATUS_UNSUCCESSFUL;
        KLErr("SocketSend failed Sent %d", Sent);
        goto socket_close;
    }

    Status = SocketReceive(Socket, HttpResponse, sizeof(HttpResponse), &Received, &Disconnected);
    if (!NT_SUCCESS(Status)) {
        KLErr("SocketReceive failed Status 0x%x", Status);
        goto socket_close;
    }

    if ((Received != sizeof(HttpResponse)) || Disconnected) {
        KLErr("Received %d Disconnected %d", Received, Disconnected);
        Status = STATUS_UNSUCCESSFUL;
        goto socket_close;
    }

    if (4 != RtlCompareMemory(HttpResponse, "HTTP", 4)) {
        KLErr("Incorrect data received");
        Status = STATUS_UNSUCCESSFUL;
        goto socket_close;
    }

    Status = STATUS_SUCCESS;
socket_close:
    SocketClose(Socket);
socket_factory_release:
    SocketFactoryRelease(SocketFactory);
socket_factory_free:
    NpFree(SocketFactory, TESTS_TAG);
    KLInf("SocketTest Status 0x%x", Status);
    return Status;
}

NTSTATUS MapTest(PVOID Context)
{
    typedef struct _SID_KV {
        CHAR *Key;
        CHAR *Value;
    } SID_KV, *PSID_KV;

    NTSTATUS Status;
    PVOID Value;
    PMAP Map;
    ULONG Index, ValueSize;
    SID_KV TestKvs[] = {{"S-1-0-0", "SID0"},
                        {"S-1-1-0", "SID1"},
                        {"S-1-0", "SID2"},
                        {"S-1-1", "SID3"},
                        {"S-1-5-21-1180699209-877415012-3182924384-1004", "BIG-SID4"},
                        {"S-1-5-20", "SID5"},
                        {"S-1-5-21-1180699209-111115012-3182924384-1004", "Another-BIG-SID6"}};
    Map = MapCreate();
    if (!Map) {
        KLErr("Can't create map");
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    for (Index = 0; Index < RTL_NUMBER_OF(TestKvs); Index++) {
        Status = MapInsertKey(Map, TestKvs[Index].Key, (ULONG)(strlen(TestKvs[Index].Key) + 1),
                              TestKvs[Index].Value, (ULONG)(strlen(TestKvs[Index].Value) + 1));
        if (!NT_SUCCESS(Status)) {
            KLErr("MapInsertKey failed Status 0x%x", Status);
            goto cleanup;
        }
    }

    /* Try to insert key 4 again */
    Status = MapInsertKey(Map, TestKvs[4].Key, (ULONG)(strlen(TestKvs[4].Key) + 1),
                          TestKvs[4].Value, (ULONG)(strlen(TestKvs[4].Value) + 1));
    if (Status != STATUS_OBJECT_NAME_COLLISION) {
        Status = STATUS_UNSUCCESSFUL;
        KLErr("MapInsertKey duplicate Status 0x%x", Status);
        goto cleanup;
    }

    /* Delete key 5 */
    Status = MapDeleteKey(Map, TestKvs[5].Key, (ULONG)(strlen(TestKvs[5].Key) + 1));
    if (!NT_SUCCESS(Status)) {
        KLErr("MapDeleteKey Status 0x%x", Status);
        goto cleanup;
    }

    for (Index = 0; Index < RTL_NUMBER_OF(TestKvs); Index++) {
        /* Skip key 5 because it's already deleted */
        if (Index == 5)
            continue;
        Status = MapLookupKey(Map, TestKvs[Index].Key, (ULONG)(strlen(TestKvs[Index].Key) + 1), &Value, &ValueSize);
        if (!NT_SUCCESS(Status)) {
            KLErr("MapLookupKey failed Status 0x%x", Status);
            goto cleanup;
        }

        if (ValueSize != (strlen(TestKvs[Index].Value) + 1)) {
            KLErr("MapLookupKey returned invalid ValueSize 0x%x", ValueSize);
            ExFreePoolWithTag(Value, MAP_TAG);
            Status = STATUS_UNSUCCESSFUL;
            goto cleanup;
        }
        if (ValueSize != RtlCompareMemory(Value, TestKvs[Index].Value, ValueSize)) {
            KLErr("MapLookupKey returned invalid Value content");
            ExFreePoolWithTag(Value, MAP_TAG);
            Status = STATUS_UNSUCCESSFUL;
            goto cleanup;            
        }
        ExFreePoolWithTag(Value, MAP_TAG);
    }
    Status = STATUS_SUCCESS;
cleanup:
    MapDelete(Map);
    KLInf("MapTest Status 0x%x", Status);
    return Status;
}

NTSTATUS RunTests(VOID)
{
    NTSTATUS Status;
    WORKER Worker;

    Status = WorkerStart(&Worker);
    if (!NT_SUCCESS(Status)) {
        KLErr("WorkerStart failed Status 0x%x", Status);
        return Status;
    }

    Status = WorkerExec(&Worker, SocketTest, NULL, TRUE);
    if (!NT_SUCCESS(Status)) {
        KLErr("SocketTest failed Status 0x%x", Status);
        goto cleanup;
    }

    Status = WorkerExec(&Worker, MapTest, NULL, TRUE);
    if (!NT_SUCCESS(Status)) {
        KLErr("MapTest failed Status 0x%x", Status);
        goto cleanup;
    }

cleanup:
    KLInf("RunTests Status 0x%x", Status);
    WorkerStop(&Worker);
    return Status;
}