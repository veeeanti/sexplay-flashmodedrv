/*++

Module Name:

    public.h

Abstract:

    This module contains the common declarations shared by driver
    and user applications.

Environment:

    user and kernel

--*/

//
// Define an Interface Guid so that app can find the device and talk to it.
//

DEFINE_GUID (GUID_DEVINTERFACE_SEXPlayFlashmodeDrv,
    0xd49bdfeb,0x979c,0x40c0,0x80,0x5e,0x27,0xc2,0x1c,0xb5,0x14,0xa2);
// {d49bdfeb-979c-40c0-805e-27c21cb514a2}
