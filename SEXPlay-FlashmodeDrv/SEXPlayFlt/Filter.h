/*++

Module Name:

    filter.h - Device filter driver definitions.

Abstract:

    This file contains the common declarations shared by
    the filter driver and user applications.

Environment:

    Kernel-mode Driver Framework

--*/

#ifndef _FILTER_H_
#define _FILTER_H_

#include <ntddk.h>
#include <wdf.h>
#include <usb.h>
#include <usbdlib.h>
#include <wdfusb.h>

//
// Define the device interface GUID for flash mode device
//
DEFINE_GUID (GUID_DEVINTERFACE_SEXPLAYFLT,
    0xD49BDFEB, 0x979C, 0x40C0, 0x80, 0x5E, 0x27, 0xC2, 0x1C, 0xB5, 0x14, 0xA3);
// {D49BDFEB-979C-40C0-805E-27C21CB514A3}

//
// Context structure for filter device
//
typedef struct _FILTER_DEVICE_CONTEXT
{
    WDFDEVICE WdfFilterDevice;
    WDFUSBDEVICE UsbDevice;

} FILTER_DEVICE_CONTEXT, *PFILTER_DEVICE_CONTEXT;

WDF_DECLARE_CONTEXT_TYPE_WITH_NAME(FILTER_DEVICE_CONTEXT, FilterGetContext)

//
// Function prototypes
//

DRIVER_INITIALIZATION DriverEntry;
EVT_WDF_DRIVER_DEVICE_ADD FilterEvtDeviceAdd;
EVT_WDF_OBJECT_CONTEXT_CLEANUP FilterEvtDriverContextCleanup;
EVT_WDF_DEVICE_PREPARE_HARDWARE FilterEvtDevicePrepareHardware;
EVT_WDF_DEVICE_RELEASE_HARDWARE FilterEvtDeviceReleaseHardware;
EVT_WDF_DEVICE_D0_EXIT FilterEvtDeviceD0Exit;
EVT_WDF_DEVICE_D0_ENTRY FilterEvtDeviceD0Entry;

//
// Filter I/O callback prototypes
//
EVT_WDF_IO_QUEUE_IO_READ FilterEvtIoRead;
EVT_WDF_IO_QUEUE_IO_WRITE FilterEvtIoWrite;
EVT_WDF_IO_QUEUE_IO_DEVICE_CONTROL FilterEvtIoDeviceControl;
EVT_WDF_IO_QUEUE_IO_DEFAULT FilterEvtIoDefault;
EVT_WDF_DEVICE_FILE_CREATE FilterEvtDeviceFileCreate;
EVT_WDF_FILE_CLOSE FilterEvtFileClose;

//
// Filter queue functions
//
NTSTATUS
FilterQueueInitialize(
    _In_ WDFDEVICE Device
    );

//
// Device attributes
//
#define FLT_POOL_TAG 'tplF'

#endif // _FILTER_H_
