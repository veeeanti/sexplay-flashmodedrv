#pragma once

#include <ntdef.h>
#include <wdf.h>

EXTERN_C_START

//
// This is the context that can be placed per queue
// and would contain per queue information.
//
typedef struct _QUEUE_CONTEXT {

    ULONG PrivateDeviceData;  // placeholder
    WDFUSBPIPE ReadPipe;     // Bulk IN pipe handle
    WDFUSBPIPE WritePipe;     // Bulk OUT pipe handle

} QUEUE_CONTEXT, *PQUEUE_CONTEXT;

WDF_DECLARE_CONTEXT_TYPE_WITH_NAME(QUEUE_CONTEXT, QueueGetContext)

NTSTATUS
SEXPlayFlashmodeDrvQueueInitialize(
    _In_ WDFDEVICE Device
    );

//
// Events from the IoQueue object
//
EVT_WDF_IO_QUEUE_IO_DEVICE_CONTROL SEXPlayFlashmodeDrvEvtIoDeviceControl;
EVT_WDF_IO_QUEUE_IO_STOP SEXPlayFlashmodeDrvEvtIoStop;
EVT_WDF_IO_QUEUE_IO_READ SEXPlayFlashmodeDrvEvtIoRead;
EVT_WDF_IO_QUEUE_IO_WRITE SEXPlayFlashmodeDrvEvtIoWrite;

//
// USB pipe management functions
//
VOID
SEXPlayFlashmodeDrvInitializePipes(
    _In_ WDFDEVICE Device
    );

EXTERN_C_END
