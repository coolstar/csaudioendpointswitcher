#include <windows.h>
#include <hidsdi.h>
#include <setupapi.h>
#include <stdio.h>
#include <stdlib.h>

#include "csaudioclient.h"

#if __GNUC__
#define __in
#define __in_ecount(x)
typedef void* PVOID;
typedef PVOID HDEVINFO;
WINHIDSDI BOOL WINAPI HidD_SetOutputReport(HANDLE, PVOID, ULONG);
#endif

typedef struct _csaudio_client_t
{
	HANDLE hSettings;
} csaudio_client_t;

//
// Function prototypes
//

HANDLE
SearchMatchingHwID(
	USAGE myUsagePage,
	USAGE myUsage
	);

HANDLE
OpenDeviceInterface(
	HDEVINFO HardwareDeviceInfo,
	PSP_DEVICE_INTERFACE_DATA DeviceInterfaceData,
	USAGE myUsagePage,
	USAGE myUsage
	);

BOOLEAN
CheckIfOurDevice(
	HANDLE file,
	USAGE myUsagePage,
	USAGE myUsage
	);

BOOL
HidOutput(
	BOOL useSetOutputReport,
	HANDLE file,
	PCHAR buffer,
	ULONG bufferSize
	);

//
// Copied this structure from hidport.h
//

typedef struct _HID_DEVICE_ATTRIBUTES {

	ULONG           Size;
	//
	// sizeof (struct _HID_DEVICE_ATTRIBUTES)
	//
	//
	// Vendor ids of this hid device
	//
	USHORT          VendorID;
	USHORT          ProductID;
	USHORT          VersionNumber;
	USHORT          Reserved[11];

} HID_DEVICE_ATTRIBUTES, *PHID_DEVICE_ATTRIBUTES;

static USHORT TpVendorID = 0;

USHORT getVendorID() {
	return TpVendorID;
}

//
// Implementation
//

pcsaudio_client csaudio_alloc(void)
{
	return (pcsaudio_client)malloc(sizeof(csaudio_client_t));
}

void csaudio_free(pcsaudio_client csaudio)
{
	free(csaudio);
}

BOOL csaudio_connect(pcsaudio_client csaudio)
{
	//
	// Find the HID devices
	//

	csaudio->hSettings = SearchMatchingHwID(0xff00, 0x0004);
	if (csaudio->hSettings == INVALID_HANDLE_VALUE || csaudio->hSettings == NULL)
	{
		csaudio_disconnect(csaudio);
		return FALSE;
	}

	//
	// Set the buffer count to 10 on the setting HID
	//

	if (!HidD_SetNumInputBuffers(csaudio->hSettings, 10))
	{
		printf("failed HidD_SetNumInputBuffers %d\n", GetLastError());
		csaudio_disconnect(csaudio);
		return FALSE;
	}

	return TRUE;
}

void csaudio_disconnect(pcsaudio_client csaudio)
{
	if (csaudio->hSettings != NULL)
		CloseHandle(csaudio->hSettings);
	csaudio->hSettings = NULL;
}

BOOL csaudio_read_message(pcsaudio_client vmulti, CsAudioSpecialKeyReport* pReport)
{
	ULONG bytesRead;

	//
	// Read the report
	//

	if (!ReadFile(vmulti->hSettings, pReport, sizeof(CsAudioSpecialKeyReport), &bytesRead, NULL))
	{
		return FALSE;
	}

	return TRUE;
}

/*BOOL csaudio_write_message(pcsaudio_client csaudio, CsAudioSpecialKeyReport* pReport)
{
	ULONG bytesWritten;

	//
	// Write the report
	//

	if (!WriteFile(csaudio->hSettings, pReport, sizeof(CsAudioSpecialKeyReport), &bytesWritten, NULL))
	{
		printf("failed WriteFile %d\n", GetLastError());
		return FALSE;
	}

	return TRUE;
}*/

HANDLE
SearchMatchingHwID(
	USAGE myUsagePage,
	USAGE myUsage
	)
{
	HDEVINFO                  hardwareDeviceInfo;
	SP_DEVICE_INTERFACE_DATA  deviceInterfaceData;
	SP_DEVINFO_DATA           devInfoData;
	GUID                      hidguid;
	int                       i;

	HidD_GetHidGuid(&hidguid);

	hardwareDeviceInfo =
		SetupDiGetClassDevs((LPGUID)&hidguid,
			NULL,
			NULL, // Define no
			(DIGCF_PRESENT |
				DIGCF_INTERFACEDEVICE));

	if (INVALID_HANDLE_VALUE == hardwareDeviceInfo)
	{
		printf("SetupDiGetClassDevs failed: %x\n", GetLastError());
		return INVALID_HANDLE_VALUE;
	}

	deviceInterfaceData.cbSize = sizeof(SP_DEVICE_INTERFACE_DATA);

	devInfoData.cbSize = sizeof(SP_DEVINFO_DATA);

	//
	// Enumerate devices of this interface class
	//

	printf("\n....looking for our HID device (with UP=0x%x "
		"and Usage=0x%x)\n", myUsagePage, myUsage);

	for (i = 0; SetupDiEnumDeviceInterfaces(hardwareDeviceInfo,
		0, // No care about specific PDOs
		(LPGUID)&hidguid,
		i, //
		&deviceInterfaceData);
	i++)
	{

		//
		// Open the device interface and Check if it is our device
		// by matching the Usage page and Usage from Hid_Caps.
		// If this is our device then send the hid request.
		//

		HANDLE file = OpenDeviceInterface(hardwareDeviceInfo, &deviceInterfaceData, myUsagePage, myUsage);

		if (file != INVALID_HANDLE_VALUE)
		{
			SetupDiDestroyDeviceInfoList(hardwareDeviceInfo);
			return file;
		}

		//
		//device was not found so loop around.
		//

	}

	printf("Failure: Could not find our HID device \n");

	SetupDiDestroyDeviceInfoList(hardwareDeviceInfo);

	return INVALID_HANDLE_VALUE;
}

HANDLE
OpenDeviceInterface(
	HDEVINFO hardwareDeviceInfo,
	PSP_DEVICE_INTERFACE_DATA deviceInterfaceData,
	USAGE myUsagePage,
	USAGE myUsage
	)
{
	PSP_DEVICE_INTERFACE_DETAIL_DATA    deviceInterfaceDetailData = NULL;

	DWORD        predictedLength = 0;
	DWORD        requiredLength = 0;
	HANDLE       file = INVALID_HANDLE_VALUE;

	SetupDiGetDeviceInterfaceDetail(
		hardwareDeviceInfo,
		deviceInterfaceData,
		NULL, // probing so no output buffer yet
		0, // probing so output buffer length of zero
		&requiredLength,
		NULL
		); // not interested in the specific dev-node

	predictedLength = requiredLength;

	deviceInterfaceDetailData =
		(PSP_DEVICE_INTERFACE_DETAIL_DATA)malloc(predictedLength);

	if (!deviceInterfaceDetailData)
	{
		printf("Error: OpenDeviceInterface: malloc failed\n");
		goto cleanup;
	}

	deviceInterfaceDetailData->cbSize =
		sizeof(SP_DEVICE_INTERFACE_DETAIL_DATA);

	if (!SetupDiGetDeviceInterfaceDetail(
		hardwareDeviceInfo,
		deviceInterfaceData,
		deviceInterfaceDetailData,
		predictedLength,
		&requiredLength,
		NULL))
	{
		printf("Error: SetupDiGetInterfaceDeviceDetail failed\n");
		free(deviceInterfaceDetailData);
		goto cleanup;
	}

	file = CreateFile(deviceInterfaceDetailData->DevicePath,
		GENERIC_READ | GENERIC_WRITE,
		FILE_SHARE_READ | FILE_SHARE_WRITE,
		NULL, // no SECURITY_ATTRIBUTES structure
		OPEN_EXISTING, // No special create flags
		0, // No special attributes
		NULL); // No template file

	if (INVALID_HANDLE_VALUE == file) {
		printf("Error: CreateFile failed: %d\n", GetLastError());
		goto cleanup;
	}

	if (CheckIfOurDevice(file, myUsagePage, myUsage)) {

		goto cleanup;

	}

	CloseHandle(file);

	file = INVALID_HANDLE_VALUE;

cleanup:

	free(deviceInterfaceDetailData);

	return file;

}


BOOLEAN
CheckIfOurDevice(
	HANDLE file,
	USAGE myUsagePage,
	USAGE myUsage)
{
	PHIDP_PREPARSED_DATA Ppd = NULL; // The opaque parser info describing this device
	HIDD_ATTRIBUTES                 Attributes; // The Attributes of this hid device.
	HIDP_CAPS                       Caps; // The Capabilities of this hid device.
	BOOLEAN                         result = FALSE;

	if (!HidD_GetPreparsedData(file, &Ppd))
	{
		printf("Error: HidD_GetPreparsedData failed \n");
		goto cleanup;
	}

	if (!HidD_GetAttributes(file, &Attributes))
	{
		printf("Error: HidD_GetAttributes failed \n");
		goto cleanup;
	}

	if ((Attributes.VendorID == RT5682_VID && Attributes.ProductID == RT5682_PID) ||
		(Attributes.VendorID == RT5682_VID2 && Attributes.ProductID == RT5682_PID2) ||
		(Attributes.VendorID == NAU8825_VID && Attributes.ProductID == NAU8825_PID) ||
		(Attributes.VendorID == DA7219_VID && Attributes.ProductID == DA7219_PID))
	{
		TpVendorID = Attributes.VendorID;
		
		if (!HidP_GetCaps(Ppd, &Caps))
		{
			printf("Error: HidP_GetCaps failed \n");
			goto cleanup;
		}

		if ((Caps.UsagePage == myUsagePage) && (Caps.Usage == myUsage))
		{
			printf("Success: Found my device.. \n");
			result = TRUE;
		}
	}

cleanup:

	if (Ppd != NULL)
	{
		HidD_FreePreparsedData(Ppd);
	}

	return result;
}

BOOL
HidOutput(
	BOOL useSetOutputReport,
	HANDLE file,
	PCHAR buffer,
	ULONG bufferSize
	)
{
	ULONG bytesWritten;
	if (useSetOutputReport)
	{
		//
		// Send Hid report thru HidD_SetOutputReport API
		//

		if (!HidD_SetOutputReport(file, buffer, bufferSize))
		{
			printf("failed HidD_SetOutputReport %d\n", GetLastError());
			return FALSE;
		}
	}
	else
	{
		if (!WriteFile(file, buffer, bufferSize, &bytesWritten, NULL))
		{
			printf("failed WriteFile %d\n", GetLastError());
			return FALSE;
		}
	}

	return TRUE;
}