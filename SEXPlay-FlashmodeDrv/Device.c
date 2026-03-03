/*++

Module Name:

    device.c - Device handling events for SEXPlay flashmode driver.

Abstract:

   This file contains the device entry points and callbacks.
   Based on GordonGate ggsemc driver for Sony Ericsson devices.
     
Environment:

    Kernel-mode Driver Framework

--*/

#include "driver.h"
#include "device.tmh"

#ifdef ALLOC_PRAGMA
#pragma alloc_text (PAGE, SEXPlayFlashmodeDrvCreateDevice)
#pragma alloc_text (PAGE, SEXPlayFlashmodeDrvEvtDevicePrepareHardware)
#pragma alloc_text (PAGE, SEXPlayFlashmodeDrvEvtDeviceReleaseHardware)
#endif

NTSTATUS
SEXPlayFlashmodeDrvCreateDevice(
    _Inout_ PWDFDEVICE_INIT DeviceInit
    )
/*++

Routine Description:

    Worker routine called to create a device and its software resources.

Arguments:

    DeviceInit - Pointer to an opaque init structure. Memory for this
                    structure will be freed by the framework when the WdfDeviceCreate
                    succeeds. So don't access the structure after that point.

Return Value:

    NTSTATUS

--*/
{
    WDF_PNPPOWER_EVENT_CALLBACKS pnpPowerCallbacks;
    WDF_OBJECT_ATTRIBUTES   deviceAttributes;
    PDEVICE_CONTEXT deviceContext;
    WDFDEVICE device;
    NTSTATUS status;

    PAGED_CODE();

    TraceEvents(TRACE_LEVEL_INFORMATION, TRACE_DRIVER, "%!FUNC! Entry");

    //
    // Initialize PnP/Power callbacks
    //
    WDF_PNPPOWER_EVENT_CALLBACKS_INIT(&pnpPowerCallbacks);
    pnpPowerCallbacks.EvtDevicePrepareHardware = SEXPlayFlashmodeDrvEvtDevicePrepareHardware;
    pnpPowerCallbacks.EvtDeviceReleaseHardware = SEXPlayFlashmodeDrvEvtDeviceReleaseHardware;
    pnpPowerCallbacks.EvtDeviceD0Entry = SEXPlayFlashmodeDrvEvtDeviceD0Entry;
    pnpPowerCallbacks.EvtDeviceD0Exit = SEXPlayFlashmodeDrvEvtDeviceD0Exit;
    WdfDeviceInitSetPnpPowerEventCallbacks(DeviceInit, &pnpPowerCallbacks);

    //
    // Initialize device attributes and context type
    //
    WDF_OBJECT_ATTRIBUTES_INIT_CONTEXT_TYPE(&deviceAttributes, DEVICE_CONTEXT);

    status = WdfDeviceCreate(&DeviceInit, &deviceAttributes, &device);

    if (NT_SUCCESS(status)) {
        //
        // Get a pointer to the device context structure
        //
        deviceContext = DeviceGetContext(device);

        //
        // Initialize the context to zero
        //
        RtlZeroMemory(deviceContext, sizeof(DEVICE_CONTEXT));

        //
        // Create a device interface so that applications can find and talk to us
        //
        status = WdfDeviceCreateDeviceInterface(
            device,
            &GUID_DEVINTERFACE_SEXPlayFlashmodeDrv,
            NULL // ReferenceString
            );

        if (NT_SUCCESS(status)) {
            //
            // Initialize the I/O Package and queues
            //
            status = SEXPlayFlashmodeDrvQueueInitialize(device);
        }
    }

    TraceEvents(TRACE_LEVEL_INFORMATION, TRACE_DRIVER, "%!FUNC! Exit = %!STATUS!", status);

    return status;
}

NTSTATUS
SEXPlayFlashmodeDrvEvtDevicePrepareHardware(
    _In_ WDFDEVICE Device,
    _In_ WDFCMRESLIST ResourceList,
    _In_ WDFCMRESLIST ResourceListTranslated
    )
/*++

Routine Description:

    In this callback, the driver does whatever is necessary to make the
    hardware ready to use. In the case of a USB device, this involves
    reading and selecting descriptors.

Arguments:

    Device - handle to a device

Return Value:

    NT status value

--*/
{
    NTSTATUS status;
    PDEVICE_CONTEXT pDeviceContext;
    WDF_USB_DEVICE_CREATE_CONFIG createParams;
    WDF_USB_DEVICE_SELECT_CONFIG_PARAMS configParams;
    USB_DEVICE_DESCRIPTOR deviceDescriptor;
    WDF_USB_INTERFACE_DESCRIPTOR_SET interfaceDescriptor;
    ULONG i;

    UNREFERENCED_PARAMETER(ResourceList);
    UNREFERENCED_PARAMETER(ResourceListTranslated);

    PAGED_CODE();

    TraceEvents(TRACE_LEVEL_INFORMATION, TRACE_DRIVER, "%!FUNC! Entry");

    status = STATUS_SUCCESS;
    pDeviceContext = DeviceGetContext(Device);

    //
    // Create a USB device handle
    //
    if (pDeviceContext->UsbDevice == NULL) {

        WDF_USB_DEVICE_CREATE_CONFIG_INIT(&createParams,
                                         USBD_CLIENT_CONTRACT_VERSION_602
                                         );

        status = WdfUsbTargetDeviceCreateWithParameters(Device,
                                                     &createParams,
                                                     WDF_NO_OBJECT_ATTRIBUTES,
                                                     &pDeviceContext->UsbDevice
                                                     );

        if (!NT_SUCCESS(status)) {
            TraceEvents(TRACE_LEVEL_ERROR, TRACE_DEVICE,
                        "WdfUsbTargetDeviceCreateWithParameters failed 0x%x", status);
            return status;
        }

        //
        // Get device descriptor information
        //
        status = WdfUsbTargetDeviceGetDeviceDescriptor(pDeviceContext->UsbDevice,
                                                        &deviceDescriptor);

        if (NT_SUCCESS(status)) {
            pDeviceContext->VendorId = deviceDescriptor.idVendor;
            pDeviceContext->ProductId = deviceDescriptor.idProduct;

            TraceEvents(TRACE_LEVEL_INFORMATION, TRACE_DEVICE,
                        "VID=%04x PID=%04x",
                        pDeviceContext->VendorId,
                        pDeviceContext->ProductId);
        }

        //
        // Get device speed
        //
        pDeviceContext->DeviceSpeed = WdfUsbTargetDeviceGetDeviceSpeed(pDeviceContext->UsbDevice);

        TraceEvents(TRACE_LEVEL_INFORMATION, TRACE_DEVICE,
                    "Device Speed = %d", pDeviceContext->DeviceSpeed);
    }

    //
    // Select the first configuration of the device
    //
    WDF_USB_DEVICE_SELECT_CONFIG_PARAMS_INIT_SINGLE(&configParams);

    status = WdfUsbTargetDeviceSelectConfig(pDeviceContext->UsbDevice,
                                            WDF_NO_OBJECT_ATTRIBUTES,
                                            &configParams
                                            );

    if (!NT_SUCCESS(status)) {
        TraceEvents(TRACE_LEVEL_ERROR, TRACE_DEVICE,
                    "WdfUsbTargetDeviceSelectConfig failed 0x%x", status);
        return status;
    }

    //
    // Get the interface handle
    //
    pDeviceContext->UsbInterface = WdfUsbInterfaceGetFirstInterface(pDeviceContext->UsbDevice);

    if (pDeviceContext->UsbInterface != NULL) {
        //
        // Get interface descriptor
        //
        WDF_USB_INTERFACE_DESCRIPTOR_INIT(&interfaceDescriptor, 
                                          0, // InterfaceIndex 
                                          USB_BUS_INTERFACE_VERSION_610);

        status = WdfUsbInterfaceGetDescriptor(pDeviceContext->UsbInterface, 
                                              &interfaceDescriptor);

        if (NT_SUCCESS(status)) {
            TraceEvents(TRACE_LEVEL_INFORMATION, TRACE_DEVICE,
                        "Interface: NumEndpoints=%d, InterfaceNumber=%d, AlternateSetting=%d",
                        interfaceDescriptor.NumberOfEndpoints,
                        interfaceDescriptor.InterfaceNumber,
                        interfaceDescriptor.AlternateSetting);
        }

        //
        // Find bulk endpoints
        //
        for (i = 0; i < interfaceDescriptor.NumberOfEndpoints; i++) {
            WDF_USB_ENDPOINT_DESCRIPTOR_INFORMATION endpointInfo;
            
            endpointInfo.EndpointIndex = i;
            WdfUsbInterfaceGetEndpointInformation(pDeviceContext->UsbInterface,
                                                   &endpointInfo);

            if (endpointInfo.PipeType == WdfUsbPipeTypeBulk) {
                if (endpointInfo.Direction == WdfUsbTargetDeviceRead) {
                    // Bulk IN
                    pDeviceContext->BulkInEndpoint = endpointInfo.EndpointAddress;
                    pDeviceContext->BulkInMaxPacket = endpointInfo.MaximumPacketSize;
                    TraceEvents(TRACE_LEVEL_INFORMATION, TRACE_DEVICE,
                                "Bulk IN: EP=%02x MaxPacket=%d",
                                pDeviceContext->BulkInEndpoint,
                                pDeviceContext->BulkInMaxPacket);
                } else {
                    // Bulk OUT
                    pDeviceContext->BulkOutEndpoint = endpointInfo.EndpointAddress;
                    pDeviceContext->BulkOutMaxPacket = endpointInfo.MaximumPacketSize;
                    TraceEvents(TRACE_LEVEL_INFORMATION, TRACE_DEVICE,
                                "Bulk OUT: EP=%02x MaxPacket=%d",
                                pDeviceContext->BulkOutEndpoint,
                                pDeviceContext->BulkOutMaxPacket);
                }
            }
        }
    }

    TraceEvents(TRACE_LEVEL_INFORMATION, TRACE_DRIVER, "%!FUNC! Exit = %!STATUS!", status);

    return status;
}

NTSTATUS
SEXPlayFlashmodeDrvEvtDeviceReleaseHardware(
    _In_ WDFDEVICE Device,
    _In_ WDFCMRESLIST ResourceListTranslated
    )
/*++

Routine Description:

    In this callback, the driver releases hardware resources.

Arguments:

    Device - handle to a device

Return Value:

    NT status value

--*/
{
    PDEVICE_CONTEXT pDeviceContext;

    UNREFERENCED_PARAMETER(ResourceListTranslated);

    PAGED_CODE();

    TraceEvents(TRACE_LEVEL_INFORMATION, TRACE_DRIVER, "%!FUNC! Entry");

    pDeviceContext = DeviceGetContext(Device);

    //
    // Release USB device handle (if any)
    //
    if (pDeviceContext->UsbDevice != NULL) {
        WdfObjectDelete(pDeviceContext->UsbDevice);
        pDeviceContext->UsbDevice = NULL;
    }

    TraceEvents(TRACE_LEVEL_INFORMATION, TRACE_DRIVER, "%!FUNC! Exit");

    return STATUS_SUCCESS;
}

NTSTATUS
SEXPlayFlashmodeDrvEvtDeviceD0Entry(
    _In_ WDFDEVICE Device,
    _In_ WDF_POWER_DEVICE_STATE PreviousState
    )
/*++

Routine Description:

    EvtDeviceD0Entry is called by the framework to put the device into the
    working (D0) state.

Arguments:

    Device - Handle to a framework device object.

    PreviousState - Device power state that the device was in before
                    entering D0.

Return Value:

    NTSTATUS

--*/
{
    PDEVICE_CONTEXT pDeviceContext;

    UNREFERENCED_PARAMETER(PreviousState);

    TraceEvents(TRACE_LEVEL_INFORMATION, TRACE_DRIVER, "%!FUNC! Entry");

    pDeviceContext = DeviceGetContext(Device);

    //
    // Device is now powered on
    //

    TraceEvents(TRACE_LEVEL_INFORMATION, TRACE_DRIVER, "%!FUNC! Exit");

    return STATUS_SUCCESS;
}

NTSTATUS
SEXPlayFlashmodeDrvEvtDeviceD0Exit(
    _In_ WDFDEVICE Device,
    _In_ WDF_POWER_DEVICE_STATE TargetState
    )
/*++

Routine Description:

    EvtDeviceD0Exit is called by the framework to transition the device to
    a lower power (non-D0) state.

Arguments:

    Device - Handle to a framework device object.

    TargetState - Device power state which the device is transitioning to.

Return Value:

    NTSTATUS

--*/
{
    PDEVICE_CONTEXT pDeviceContext;

    UNREFERENCED_PARAMETER(TargetState);

    TraceEvents(TRACE_LEVEL_INFORMATION, TRACE_DRIVER, "%!FUNC! Entry");

    pDeviceContext = DeviceGetContext(Device);

    //
    // Device is transitioning to a lower power state
    //

    TraceEvents(TRACE_LEVEL_INFORMATION, TRACE_DRIVER, "%!FUNC! Exit");

    return STATUS_SUCCESS;
}
