/*++

Module Name:

    Filter.c - Filter driver entry points and callbacks.

Abstract:

    This file contains the filter driver entry points and callbacks.

Environment:

    Kernel-mode Driver Framework

--*/

#include "Filter.h"
#include "Filter.tmh"

#ifdef ALLOC_PRAGMA
#pragma alloc_text (INIT, DriverEntry)
#pragma alloc_text (PAGE, FilterEvtDeviceAdd)
#pragma alloc_text (PAGE, FilterEvtDriverContextCleanup)
#pragma alloc_text (PAGE, FilterCreateDevice)
#endif

DRIVER_INITIALIZATION DriverEntry;

NTSTATUS
DriverEntry(
    _In_ PDRIVER_OBJECT DriverObject,
    _In_ PUNICODE_STRING RegistryPath
    )
/*++

Routine Description:

    DriverEntry initializes the filter driver and is the first routine called
    by the system after the driver is loaded.

Parameters Description:

    DriverObject - represents the instance of the function driver that is loaded
    into memory.

    RegistryPath - represents the driver specific path in the Registry.

Return Value:

    STATUS_SUCCESS if successful,
    STATUS_UNSUCCESSFUL otherwise.

--*/
{
    WDF_DRIVER_CONFIG config;
    NTSTATUS status;
    WDF_OBJECT_ATTRIBUTES attributes;

    //
    // Initialize WPP Tracing
    //
    WPP_INIT_TRACING(DriverObject, RegistryPath);

    TraceEvents(TRACE_LEVEL_INFORMATION, TRACE_DRIVER, "%!FUNC! Entry");

    //
    // Register a cleanup callback so that we can call WPP_CLEANUP when
    // the framework driver object is deleted during driver unload.
    //
    WDF_OBJECT_ATTRIBUTES_INIT(&attributes);
    attributes.EvtCleanupCallback = FilterEvtDriverContextCleanup;

    WDF_DRIVER_CONFIG_INIT(&config,
                           FilterEvtDeviceAdd
                           );

    //
    // Set filter driver attributes
    //
    config.DriverPoolTag = FLT_POOL_TAG;

    status = WdfDriverCreate(DriverObject,
                             RegistryPath,
                             &attributes,
                             &config,
                             WDF_NO_HANDLE
                             );

    if (!NT_SUCCESS(status)) {
        TraceEvents(TRACE_LEVEL_ERROR, TRACE_DRIVER, "WdfDriverCreate failed %!STATUS!", status);
        WPP_CLEANUP(DriverObject);
        return status;
    }

    TraceEvents(TRACE_LEVEL_INFORMATION, TRACE_DRIVER, "%!FUNC! Exit");

    return status;
}

NTSTATUS
FilterEvtDeviceAdd(
    _In_    WDFDRIVER Driver,
    _Inout_ PWDFDEVICE_INIT DeviceInit
    )
/*++

Routine Description:

    EvtDeviceAdd is called by the framework in response to AddDevice
    call from the PnP manager. We create and initialize a device object to
    represent a new instance of the device.

Arguments:

    Driver - Handle to a framework driver object created in DriverEntry

    DeviceInit - Pointer to a framework-allocated WDFDEVICE_INIT structure.

Return Value:

    NTSTATUS

--*/
{
    NTSTATUS status;

    UNREFERENCED_PARAMETER(Driver);

    PAGED_CODE();

    TraceEvents(TRACE_LEVEL_INFORMATION, TRACE_DRIVER, "%!FUNC! Entry");

    status = FilterCreateDevice(DeviceInit);

    TraceEvents(TRACE_LEVEL_INFORMATION, TRACE_DRIVER, "%!FUNC! Exit");

    return status;
}

VOID
FilterEvtDriverContextCleanup(
    _In_ WDFOBJECT DriverObject
    )
/*++

Routine Description:

    Free all the resources allocated in DriverEntry.

Arguments:

    DriverObject - handle to a WDF Driver object.

Return Value:

    VOID.

--*/
{
    UNREFERENCED_PARAMETER(DriverObject);

    PAGED_CODE ();

    TraceEvents(TRACE_LEVEL_INFORMATION, TRACE_DRIVER, "%!FUNC! Entry");

    //
    // Stop WPP Tracing
    //
    WPP_CLEANUP(WdfDriverWdmGetDriverObject((WDFDRIVER)DriverObject));
}

NTSTATUS
FilterCreateDevice(
    _Inout_ PWDFDEVICE_INIT DeviceInit
    )
/*++

Routine Description:

    Worker routine called to create a filter device.

Arguments:

    DeviceInit - Pointer to an opaque init structure.

Return Value:

    NTSTATUS

--*/
{
    WDF_PNPPOWER_EVENT_CALLBACKS pnpPowerCallbacks;
    WDF_OBJECT_ATTRIBUTES deviceAttributes;
    WDFDEVICE device;
    NTSTATUS status;

    PAGED_CODE();

    //
    // Set filter type
    //
    WdfFdoInitSetFilterType(DeviceInit);

    //
    // Setup PnP/Power callbacks
    //
    WDF_PNPPOWER_EVENT_CALLBACKS_INIT(&pnpPowerCallbacks);
    pnpPowerCallbacks.EvtDevicePrepareHardware = FilterEvtDevicePrepareHardware;
    pnpPowerCallbacks.EvtDeviceReleaseHardware = FilterEvtDeviceReleaseHardware;
    pnpPowerCallbacks.EvtDeviceD0Entry = FilterEvtDeviceD0Entry;
    pnpPowerCallbacks.EvtDeviceD0Exit = FilterEvtDeviceD0Exit;
    WdfDeviceInitSetPnpPowerEventCallbacks(DeviceInit, &pnpPowerCallbacks);

    //
    // Initialize attributes and context
    //
    WDF_OBJECT_ATTRIBUTES_INIT_CONTEXT_TYPE(&deviceAttributes, FILTER_DEVICE_CONTEXT);
    deviceAttributes.SynchronizationScope = WdfSynchronizationScopeNone;

    status = WdfDeviceCreate(&DeviceInit, &deviceAttributes, &device);

    if (NT_SUCCESS(status)) {
        PFILTER_DEVICE_CONTEXT deviceContext;

        deviceContext = FilterGetContext(device);
        deviceContext->WdfFilterDevice = device;
        deviceContext->UsbDevice = NULL;

        //
        // Setup I/O queues
        //
        status = FilterQueueInitialize(device);
    }

    return status;
}

NTSTATUS
FilterEvtDevicePrepareHardware(
    _In_ WDFDEVICE Device,
    _In_ WDFCMRESLIST ResourceList,
    _In_ WDFCMRESLIST ResourceListTranslated
    )
/*++

Routine Description:

    In this callback, the driver does whatever is necessary to make the
    hardware ready to use.

Arguments:

    Device - handle to a device

    ResourceList - handle to a resource-list

    ResourceListTranslated - handle to a translated resource-list

Return Value:

    NT status value

--*/
{
    NTSTATUS status;
    PFILTER_DEVICE_CONTEXT deviceContext;
    WDF_USB_DEVICE_CREATE_CONFIG createParams;

    UNREFERENCED_PARAMETER(ResourceList);
    UNREFERENCED_PARAMETER(ResourceListTranslated);

    PAGED_CODE();

    TraceEvents(TRACE_LEVEL_INFORMATION, TRACE_DRIVER, "%!FUNC! Entry");

    status = STATUS_SUCCESS;
    deviceContext = FilterGetContext(Device);

    //
    // Create USB device handle
    //
    if (deviceContext->UsbDevice == NULL) {
        WDF_USB_DEVICE_CREATE_CONFIG_INIT(&createParams,
                                           USBD_CLIENT_CONTRACT_VERSION_602
                                           );

        status = WdfUsbTargetDeviceCreateWithParameters(Device,
                                                         &createParams,
                                                         WDF_NO_OBJECT_ATTRIBUTES,
                                                         &deviceContext->UsbDevice
                                                         );

        if (!NT_SUCCESS(status)) {
            TraceEvents(TRACE_LEVEL_ERROR, TRACE_DEVICE,
                        "WdfUsbTargetDeviceCreateWithParameters failed 0x%x", status);
            return status;
        }
    }

    TraceEvents(TRACE_LEVEL_INFORMATION, TRACE_DRIVER, "%!FUNC! Exit");

    return status;
}

NTSTATUS
FilterEvtDeviceReleaseHardware(
    _In_ WDFDEVICE Device,
    _In_ WDFCMRESLIST ResourceListTranslated
    )
/*++

Routine Description:

    In this callback, the driver releases hardware resources.

Arguments:

    Device - handle to a device

    ResourceListTranslated - handle to a translated resource-list

Return Value:

    NT status value

--*/
{
    PFILTER_DEVICE_CONTEXT deviceContext;

    UNREFERENCED_PARAMETER(ResourceListTranslated);

    PAGED_CODE();

    TraceEvents(TRACE_LEVEL_INFORMATION, TRACE_DRIVER, "%!FUNC! Entry");

    deviceContext = FilterGetContext(Device);

    //
    // Release USB device handle
    //
    if (deviceContext->UsbDevice != NULL) {
        WdfObjectDelete(deviceContext->UsbDevice);
        deviceContext->UsbDevice = NULL;
    }

    TraceEvents(TRACE_LEVEL_INFORMATION, TRACE_DRIVER, "%!FUNC! Exit");

    return STATUS_SUCCESS;
}

NTSTATUS
FilterEvtDeviceD0Entry(
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
    UNREFERENCED_PARAMETER(Device);
    UNREFERENCED_PARAMETER(PreviousState);

    TraceEvents(TRACE_LEVEL_INFORMATION, TRACE_DRIVER, "%!FUNC! Entry");

    return STATUS_SUCCESS;
}

NTSTATUS
FilterEvtDeviceD0Exit(
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
    UNREFERENCED_PARAMETER(Device);
    UNREFERENCED_PARAMETER(TargetState);

    TraceEvents(TRACE_LEVEL_INFORMATION, TRACE_DRIVER, "%!FUNC! Entry");

    return STATUS_SUCCESS;
}
