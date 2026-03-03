/*++

Module Name:

    FilterQueue.c - Filter driver queue callbacks.

Abstract:

    This file contains the queue entry points and callbacks.

Environment:

    Kernel-mode Driver Framework

--*/

#include "Filter.h"
#include "Filter.tmh"

#ifdef ALLOC_PRAGMA
#pragma alloc_text (PAGE, FilterQueueInitialize)
#endif

NTSTATUS
FilterQueueInitialize(
    _In_ WDFDEVICE Device
    )
/*++

Routine Description:

    The I/O dispatch callbacks for the filter driver are configured in this function.

    A single default I/O Queue is configured for parallel request processing.

Arguments:

    Device - Handle to a framework device object.

Return Value:

    VOID

--*/
{
    WDFQUEUE queue;
    NTSTATUS status;
    WDF_IO_QUEUE_CONFIG queueConfig;

    PAGED_CODE();

    TraceEvents(TRACE_LEVEL_INFORMATION, TRACE_QUEUE, "%!FUNC! Entry");

    //
    // Configure a default queue for filter driver
    //
    WDF_IO_QUEUE_CONFIG_INIT_DEFAULT_QUEUE(
         &queueConfig,
        WdfIoQueueDispatchParallel
        );

    queueConfig.EvtIoDefault = FilterEvtIoDefault;
    queueConfig.EvtIoRead = FilterEvtIoRead;
    queueConfig.EvtIoWrite = FilterEvtIoWrite;
    queueConfig.EvtIoDeviceControl = FilterEvtIoDeviceControl;

    status = WdfIoQueueCreate(
                 Device,
                 &queueConfig,
                 WDF_NO_OBJECT_ATTRIBUTES,
                 &queue
                 );

    if (!NT_SUCCESS(status)) {
        TraceEvents(TRACE_LEVEL_ERROR, TRACE_QUEUE, "WdfIoQueueCreate failed %!STATUS!", status);
        return status;
    }

    TraceEvents(TRACE_LEVEL_INFORMATION, TRACE_QUEUE, "%!FUNC! Exit");

    return status;
}

VOID
FilterEvtIoDefault(
    _In_ WDFQUEUE Queue,
    _In_ WDFREQUEST Request
    )
/*++

Routine Description:

    Default I/O callback - forwards requests to lower driver.

Arguments:

    Queue - Handle to the framework queue object.

    Request - Handle to the framework request object.

Return Value:

    VOID

--*/
{
    PFILTER_DEVICE_CONTEXT deviceContext;
    WDFDEVICE device;
    NTSTATUS status;

    TraceEvents(TRACE_LEVEL_INFORMATION, TRACE_QUEUE,
                "%!FUNC! Queue 0x%p, Request 0x%p",
                Queue, Request);

    //
    // Forward the request to the lower driver
    //
    device = WdfIoQueueGetDevice(Queue);
    deviceContext = FilterGetContext(device);

    status = WdfRequestForwardToIoQueue(Request, WdfDeviceGetIoQueue(deviceContext->UsbDevice));

    if (!NT_SUCCESS(status)) {
        TraceEvents(TRACE_LEVEL_ERROR, TRACE_QUEUE,
                    "WdfRequestForwardToIoQueue failed %!STATUS!", status);
        WdfRequestComplete(Request, status);
    }

    return;
}

VOID
FilterEvtIoRead(
    _In_ WDFQUEUE Queue,
    _In_ WDFREQUEST Request,
    _In_ size_t Length
    )
/*++

Routine Description:

    Read I/O callback - forwards read requests to lower driver.

Arguments:

    Queue - Handle to the framework queue object.

    Request - Handle to the framework request object.

    Length - Length of the read request.

Return Value:

    VOID

--*/
{
    UNREFERENCED_PARAMETER(Queue);
    UNREFERENCED_PARAMETER(Length);

    TraceEvents(TRACE_LEVEL_INFORMATION, TRACE_QUEUE,
                "%!FUNC! Queue 0x%p, Request 0x%p, Length %Iu",
                Queue, Request, Length);

    //
    // Complete with success - actual I/O handled by lower driver
    //
    WdfRequestComplete(Request, STATUS_SUCCESS);

    return;
}

VOID
FilterEvtIoWrite(
    _In_ WDFQUEUE Queue,
    _In_ WDFREQUEST Request,
    _In_ size_t Length
    )
/*++

Routine Description:

    Write I/O callback - forwards write requests to lower driver.

Arguments:

    Queue - Handle to the framework queue object.

    Request - Handle to the framework request object.

    Length - Length of the write request.

Return Value:

    VOID

--*/
{
    UNREFERENCED_PARAMETER(Queue);
    UNREFERENCED_PARAMETER(Length);

    TraceEvents(TRACE_LEVEL_INFORMATION, TRACE_QUEUE,
                "%!FUNC! Queue 0x%p, Request 0x%p, Length %Iu",
                Queue, Request, Length);

    //
    // Complete with success - actual I/O handled by lower driver
    //
    WdfRequestComplete(Request, STATUS_SUCCESS);

    return;
}

VOID
FilterEvtIoDeviceControl(
    _In_ WDFQUEUE Queue,
    _In_ WDFREQUEST Request,
    _In_ size_t OutputBufferLength,
    _In_ size_t InputBufferLength,
    _In_ ULONG IoControlCode
    )
/*++

Routine Description:

    Device I/O control callback - handles flash mode IOCTLs.

Arguments:

    Queue - Handle to the framework queue object.

    Request - Handle to the framework request object.

    OutputBufferLength - Size of the output buffer.

    InputBufferLength - Size of the input buffer.

    IoControlCode - I/O control code.

Return Value:

    VOID

--*/
{
    PFILTER_DEVICE_CONTEXT deviceContext;
    WDFDEVICE device;
    NTSTATUS status;

    UNREFERENCED_PARAMETER(OutputBufferLength);
    UNREFERENCED_PARAMETER(InputBufferLength);

    TraceEvents(TRACE_LEVEL_INFORMATION, TRACE_QUEUE,
                "%!FUNC! Queue 0x%p, Request 0x%p IoControlCode 0x%x",
                Queue, Request, IoControlCode);

    device = WdfIoQueueGetDevice(Queue);
    deviceContext = FilterGetContext(device);

    //
    // Forward the request to the lower driver
    //
    status = WdfRequestForwardToIoQueue(Request, WdfDeviceGetIoQueue(deviceContext->UsbDevice));

    if (!NT_SUCCESS(status)) {
        TraceEvents(TRACE_LEVEL_ERROR, TRACE_QUEUE,
                    "WdfRequestForwardToIoQueue failed %!STATUS!", status);
        WdfRequestComplete(Request, status);
    }

    return;
}
