/*++

Module Name:

    queue.c - Queue handling events for SEXPlay flashmode driver.

Abstract:

    This file contains the queue entry points and callbacks.
    Based on GordonGate ggsemc driver for Sony Ericsson devices.

Environment:

    Kernel-mode Driver Framework

--*/

#include "driver.h"
#include "queue.tmh"

#ifdef ALLOC_PRAGMA
#pragma alloc_text (PAGE, SEXPlayFlashmodeDrvQueueInitialize)
#endif

NTSTATUS
SEXPlayFlashmodeDrvQueueInitialize(
    _In_ WDFDEVICE Device
    )
/*++

Routine Description:

     The I/O dispatch callbacks for the frameworks device object
     are configured in this function.

     A single default I/O Queue is configured for parallel request
     processing.

Arguments:

    Device - Handle to a framework device object.

Return Value:

    VOID

--*/
{
    WDFQUEUE queue;
    NTSTATUS status;
    WDF_IO_QUEUE_CONFIG    queueConfig;
    PQUEUE_CONTEXT queueContext;

    PAGED_CODE();

    TraceEvents(TRACE_LEVEL_INFORMATION, TRACE_QUEUE, "%!FUNC! Entry");

    //
    // Configure a default queue so that requests that are not
    // configure-forwarded using WdfDeviceConfigureRequestDispatching to goto
    // other queues get dispatched here.
    //
    WDF_IO_QUEUE_CONFIG_INIT_DEFAULT_QUEUE(
         &queueConfig,
        WdfIoQueueDispatchParallel
        );

    queueConfig.EvtIoDeviceControl = SEXPlayFlashmodeDrvEvtIoDeviceControl;
    queueConfig.EvtIoRead = SEXPlayFlashmodeDrvEvtIoRead;
    queueConfig.EvtIoWrite = SEXPlayFlashmodeDrvEvtIoWrite;
    queueConfig.EvtIoStop = SEXPlayFlashmodeDrvEvtIoStop;

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

    //
    // Get queue context and initialize
    //
    queueContext = QueueGetContext(queue);
    queueContext->ReadPipe = NULL;
    queueContext->WritePipe = NULL;

    //
    // Initialize USB pipes
    //
    SEXPlayFlashmodeDrvInitializePipes(Device);

    TraceEvents(TRACE_LEVEL_INFORMATION, TRACE_QUEUE, "%!FUNC! Exit");

    return status;
}

VOID
SEXPlayFlashmodeDrvInitializePipes(
    _In_ WDFDEVICE Device
    )
/*++

Routine Description:

    Initialize USB bulk pipes for flashmode I/O.

Arguments:

    Device - Handle to a framework device object.

Return Value:

    VOID

--*/
{
    PDEVICE_CONTEXT deviceContext;
    WDFUSBINTERFACE usbInterface;
    ULONG i;
    WDF_USB_PIPE_INFORMATION pipeInfo;

    TraceEvents(TRACE_LEVEL_INFORMATION, TRACE_QUEUE, "%!FUNC! Entry");

    deviceContext = DeviceGetContext(Device);
    usbInterface = deviceContext->UsbInterface;

    if (usbInterface == NULL) {
        TraceEvents(TRACE_LEVEL_WARNING, TRACE_QUEUE, "USB Interface not available");
        return;
    }

    //
    // Enumerate USB pipes
    //
    for (i = 0; i < WdfUsbInterfaceGetNumberOfPipes(usbInterface); i++) {
        
        WDF_USB_PIPE_INFORMATION_INIT(&pipeInfo);
        
        WdfUsbInterfaceGetPipe(usbInterface, i, &pipeInfo);

        TraceEvents(TRACE_LEVEL_INFORMATION, TRACE_QUEUE,
                    "Pipe %d: Type=%d, Direction=%d, EP=0x%02x",
                    i,
                    pipeInfo.PipeType,
                    pipeInfo.Direction,
                    pipeInfo.EndpointAddress);

        if (pipeInfo.PipeType == WdfUsbPipeTypeBulk) {
            if (pipeInfo.Direction == WdfUsbTargetDeviceRead) {
                // Bulk IN pipe
                TraceEvents(TRACE_LEVEL_INFORMATION, TRACE_QUEUE,
                            "Bulk IN pipe initialized: EP=0x%02x, MaxPacket=%d",
                            pipeInfo.EndpointAddress,
                            pipeInfo.MaximumPacketSize);
            } else {
                // Bulk OUT pipe
                TraceEvents(TRACE_LEVEL_INFORMATION, TRACE_QUEUE,
                            "Bulk OUT pipe initialized: EP=0x%02x, MaxPacket=%d",
                            pipeInfo.EndpointAddress,
                            pipeInfo.MaximumPacketSize);
            }
        }
    }

    TraceEvents(TRACE_LEVEL_INFORMATION, TRACE_QUEUE, "%!FUNC! Exit");
}

VOID
SEXPlayFlashmodeDrvEvtIoRead(
    _In_ WDFQUEUE Queue,
    _In_ WDFREQUEST Request,
    _In_ size_t Length
    )
/*++

Routine Description:

    This event is invoked when the framework receives IRP_MJ_READ request.

Arguments:

    Queue -  Handle to the framework queue object that is associated with the
             I/O request.

    Request - Handle to a framework request object.

    Length - Length of the read request.

Return Value:

    VOID

--*/
{
    PDEVICE_CONTEXT deviceContext;
    WDFDEVICE device;
    WDF_USB_REQUEST_COMPLETION_PARAMS completionParams;
    NTSTATUS status;
    size_t bytesRead = 0;

    UNREFERENCED_PARAMETER(Queue);

    TraceEvents(TRACE_LEVEL_INFORMATION, 
                TRACE_QUEUE, 
                "%!FUNC! Queue 0x%p, Request 0x%p Length %Iu", 
                Queue, Request, Length);

    device = WdfIoQueueGetDevice(Queue);
    deviceContext = DeviceGetContext(device);

    //
    // For flashmode, read operations are typically handled by the lower driver
    // Forward the request to the USB target
    //

    WdfRequestComplete(Request, STATUS_SUCCESS);

    return;
}

VOID
SEXPlayFlashmodeDrvEvtIoWrite(
    _In_ WDFQUEUE Queue,
    _In_ WDFREQUEST Request,
    _In_ size_t Length
    )
/*++

Routine Description:

    This event is invoked when the framework receives IRP_MJ_WRITE request.

Arguments:

    Queue -  Handle to the framework queue object that is associated with the
             I/O request.

    Request - Handle to a framework request object.

    Length - Length of the write request.

Return Value:

    VOID

--*/
{
    PDEVICE_CONTEXT deviceContext;
    WDFDEVICE device;

    UNREFERENCED_PARAMETER(Queue);

    TraceEvents(TRACE_LEVEL_INFORMATION, 
                TRACE_QUEUE, 
                "%!FUNC! Queue 0x%p, Request 0x%p Length %Iu", 
                Queue, Request, Length);

    device = WdfIoQueueGetDevice(Queue);
    deviceContext = DeviceGetContext(device);

    //
    // For flashmode, write operations are typically handled by the lower driver
    //

    WdfRequestComplete(Request, STATUS_SUCCESS);

    return;
}

VOID
SEXPlayFlashmodeDrvEvtIoDeviceControl(
    _In_ WDFQUEUE Queue,
    _In_ WDFREQUEST Request,
    _In_ size_t OutputBufferLength,
    _In_ size_t InputBufferLength,
    _In_ ULONG IoControlCode
    )
/*++

Routine Description:

    This event is invoked when the framework receives IRP_MJ_DEVICE_CONTROL request.

Arguments:

    Queue -  Handle to the framework queue object that is associated with the
             I/O request.

    Request - Handle to a framework request object.

    OutputBufferLength - Size of the output buffer in bytes

    InputBufferLength - Size of the input buffer in bytes

    IoControlCode - I/O control code.

Return Value:

    VOID

--*/
{
    PDEVICE_CONTEXT deviceContext;
    WDFDEVICE device;
    NTSTATUS status = STATUS_SUCCESS;
    void* inputBuffer = NULL;
    void* outputBuffer = NULL;
    size_t inputBufferLength = 0;
    size_t outputBufferLength = 0;

    TraceEvents(TRACE_LEVEL_INFORMATION, 
                TRACE_QUEUE, 
                "%!FUNC! Queue 0x%p, Request 0x%p OutputBufferLength %Iu InputBufferLength %Iu IoControlCode 0x%08x", 
                Queue, Request, (int) OutputBufferLength, (int) InputBufferLength, IoControlCode);

    device = WdfIoQueueGetDevice(Queue);
    deviceContext = DeviceGetContext(device);

    //
    // Get input/output buffers if provided
    //
    if (InputBufferLength > 0) {
        status = WdfRequestRetrieveInputBuffer(Request, 1, &inputBuffer, &inputBufferLength);
        if (!NT_SUCCESS(status)) {
            TraceEvents(TRACE_LEVEL_ERROR, TRACE_QUEUE, 
                        "WdfRequestRetrieveInputBuffer failed %!STATUS!", status);
            WdfRequestComplete(Request, status);
            return;
        }
    }

    if (OutputBufferLength > 0) {
        status = WdfRequestRetrieveOutputBuffer(Request, 1, &outputBuffer, &outputBufferLength);
        if (!NT_SUCCESS(status)) {
            TraceEvents(TRACE_LEVEL_ERROR, TRACE_QUEUE, 
                        "WdfRequestRetrieveOutputBuffer failed %!STATUS!", status);
            WdfRequestComplete(Request, status);
            return;
        }
    }

    //
    // Process flashmode-specific IOCTLs here
    // Currently we just pass through to lower driver
    //

    WdfRequestComplete(Request, STATUS_SUCCESS);

    return;
}

VOID
SEXPlayFlashmodeDrvEvtIoStop(
    _In_ WDFQUEUE Queue,
    _In_ WDFREQUEST Request,
    _In_ ULONG ActionFlags
)
/*++

Routine Description:

    This event is invoked for a power-managed queue before the device leaves the working state (D0).

Arguments:

    Queue -  Handle to the framework queue object that is associated with the
             I/O request.

    Request - Handle to a framework request object.

    ActionFlags - A bitwise OR of one or more WDF_REQUEST_STOP_ACTION_FLAGS-typed flags
                  that identify the reason that the callback function is being called
                  and whether the request is cancelable.

Return Value:

    VOID

--*/
{
    TraceEvents(TRACE_LEVEL_INFORMATION, 
                TRACE_QUEUE, 
                "%!FUNC! Queue 0x%p, Request 0x%p ActionFlags 0x%x", 
                Queue, Request, ActionFlags);

    //
    // Complete or forward the request based on ActionFlags
    //
    WdfRequestComplete(Request, STATUS_SUCCESS);

    return;
}
