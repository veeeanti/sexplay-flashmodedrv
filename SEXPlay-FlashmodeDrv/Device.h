/*++

Module Name:

    device.h

Abstract:

    This file contains the device definitions.

Environment:

    Kernel-mode Driver Framework

--*/

#include "public.h"

EXTERN_C_START

//
// Device context for flashmode driver
// This structure stores device-specific information
//
typedef struct _DEVICE_CONTEXT
{
    WDFUSBDEVICE UsbDevice;              // USB device handle
    WDFUSBINTERFACE UsbInterface;        // USB interface handle
    UCHAR ConfigurationValue;             // Current configuration value
    UCHAR NumberConfigured;               // Number of configured interfaces
    ULONG DeviceSpeed;                    // Device speed (USB_SPEED_*)
    USHORT VendorId;                      // USB Vendor ID
    USHORT ProductId;                     // USB Product ID
    
    // Flashmode-specific fields
    ULONG BulkOutMaxPacket;               // Bulk OUT endpoint max packet size
    ULONG BulkInMaxPacket;                // Bulk IN endpoint max packet size
    UCHAR BulkOutEndpoint;                // Bulk OUT endpoint address
    UCHAR BulkInEndpoint;                 // Bulk IN endpoint address
    
} DEVICE_CONTEXT, *PDEVICE_CONTEXT;

//
// This macro will generate an inline function called DeviceGetContext
// which will be used to get a pointer to the device context memory
// in a type safe manner.
//
WDF_DECLARE_CONTEXT_TYPE_WITH_NAME(DEVICE_CONTEXT, DeviceGetContext)

//
// Function to initialize the device's queues and callbacks
//
NTSTATUS
SEXPlayFlashmodeDrvCreateDevice(
    _Inout_ PWDFDEVICE_INIT DeviceInit
    );

//
// Function to select the device's USB configuration and get a WDFUSBDEVICE
// handle
//
EVT_WDF_DEVICE_PREPARE_HARDWARE SEXPlayFlashmodeDrvEvtDevicePrepareHardware;
EVT_WDF_DEVICE_RELEASE_HARDWARE SEXPlayFlashmodeDrvEvtDeviceReleaseHardware;
EVT_WDF_DEVICE_D0_ENTRY SEXPlayFlashmodeDrvEvtDeviceD0Entry;
EVT_WDF_DEVICE_D0_EXIT SEXPlayFlashmodeDrvEvtDeviceD0Exit;

EXTERN_C_END
