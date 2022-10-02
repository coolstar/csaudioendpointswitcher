#include "AudioDeviceUtil.h"
#include <PropIdl.h>
#include <functiondiscovery.h>
#include <devicetopology.h>
#include "IPolicyConfig.h"
#include <string_view>
#include <iostream>

static bool endsWith(std::wstring_view str, std::wstring_view suffix)
{
	return str.size() >= suffix.size() && 0 == str.compare(str.size() - suffix.size(), suffix.size(), suffix);
}

static bool startsWith(std::wstring_view str, std::wstring_view prefix)
{
	return str.size() >= prefix.size() && 0 == str.compare(0, prefix.size(), prefix);
}

AudioDeviceUtil::AudioDeviceUtil()
{
	HRESULT hr;
	hr = CoInitializeEx(NULL, COINIT_MULTITHREADED);
	if (!SUCCEEDED(hr)) {
		throw;
	}

	hr = CoCreateInstance(__uuidof(MMDeviceEnumerator), NULL, CLSCTX_ALL, __uuidof(IMMDeviceEnumerator), (void**)&pEnum);

	if (!SUCCEEDED(hr)) {
		throw;
	}

	findCoolstarAudio();

	if (speaker == NULL || headphones == NULL || micarray == NULL || mic == NULL) {
		std::cout << "Failed to find CoolStar Audio" << std::endl;
	}
}

IMMDevice* AudioDeviceUtil::expectedDevice(EDataFlow dataFlow, bool jackState) {
	IMMDevice* expectedDevice = NULL;
	if (dataFlow == eRender) {
		if (jackState) {
			expectedDevice = headphones;
		}
		else {
			expectedDevice = speaker;
		}
	}
	else if (dataFlow == eCapture) {
		if (jackState) {
			expectedDevice = mic;
		}
		else {
			expectedDevice = micarray;
		}
	}
	return expectedDevice;
}

bool AudioDeviceUtil::defaultIsExpected(EDataFlow dataFlow, bool jackState, ERole role) {
	HRESULT hr;
	IMMDevice* defaultEndpoint;
	LPWSTR defaultID = (LPWSTR)L"";

	hr = pEnum->GetDefaultAudioEndpoint(dataFlow, role, &defaultEndpoint);
	if (SUCCEEDED(hr)) {
		defaultEndpoint->GetId(&defaultID);
	}
	else {
		return false;
	}

	IMMDevice* expectedDevice = this->expectedDevice(dataFlow, jackState);
	if (!expectedDevice) {
		return false;
	}

	LPWSTR expectedID = (LPWSTR)L"";
	expectedDevice->GetId(&expectedID);

	if (lstrcmpW(expectedID, defaultID) == 0)
		return true;
	return false;
}

bool AudioDeviceUtil::setExpectedDevice(EDataFlow dataFlow, bool jackState, ERole role) {
	HRESULT hr;

	IMMDevice* expectedDevice = this->expectedDevice(dataFlow, jackState);
	if (!expectedDevice) {
		return false;
	}

	LPWSTR expectedID = (LPWSTR)L"";
	expectedDevice->GetId(&expectedID);

	IPolicyConfigWin7* pPolicyConfig = nullptr;
	hr = CoCreateInstance(__uuidof(CPolicyConfigClient), NULL, CLSCTX_ALL, __uuidof(IPolicyConfig10_1), (LPVOID*)&pPolicyConfig);
	if (pPolicyConfig == nullptr) {
		hr = CoCreateInstance(__uuidof(CPolicyConfigClient), NULL, CLSCTX_ALL, __uuidof(IPolicyConfig10), (LPVOID*)&pPolicyConfig);
	}
	if (pPolicyConfig == nullptr) {
		hr = CoCreateInstance(__uuidof(CPolicyConfigClient), NULL, CLSCTX_ALL, __uuidof(IPolicyConfig7), (LPVOID*)&pPolicyConfig);
	}
	if (pPolicyConfig != NULL) {
		hr = pPolicyConfig->SetDefaultEndpoint(expectedID, role);
		pPolicyConfig->Release();
		return SUCCEEDED(hr);
	}
	return false;
}

void AudioDeviceUtil::findCoolstarAudio()
{
	HRESULT hr;
	IMMDeviceCollection* devices;

	hr = pEnum->EnumAudioEndpoints(eRender, DEVICE_STATE_ACTIVE, &devices);
	if (!SUCCEEDED(hr)) {
		return;
	}

	handleDevices(eRender, devices);

	hr = pEnum->EnumAudioEndpoints(eCapture, DEVICE_STATE_ACTIVE, &devices);
	if (!SUCCEEDED(hr)) {
		return;
	}

	handleDevices(eCapture, devices);
}

void AudioDeviceUtil::handleDevices(EDataFlow dataFlow, IMMDeviceCollection* devices)
{
	HRESULT hr;

	UINT count = 0;
	devices->GetCount(&count);

	for (UINT i = 0; i < count; i++) {
		IMMDevice* device;
		hr = devices->Item(i, &device);
		if (!SUCCEEDED(hr))
			continue;

		{
			IPropertyStore* store;
			hr = device->OpenPropertyStore(STGM_READ, &store);
			if (!SUCCEEDED(hr))
				continue;

			PROPVARIANT pv;
			PropVariantInit(&pv);

			hr = store->GetValue(PKEY_Device_FriendlyName, &pv);
			if (!SUCCEEDED(hr)) {
				PropVariantClear(&pv);
				continue;
			}

			std::wstring comp[] =
			{
				L"CoolStar ACP Audio (WDM))",
				L"CoolStar SST Audio (WDM))",
				L"CoolStar I2S Audio (WDM))",
				L"CoolStar SOF Audio (WDM))",
				L"CoolStar Audio (WDM))"
			};

			PWSTR p;
			PSFormatForDisplayAlloc(PKEY_Device_FriendlyName, pv, PDFF_DEFAULT, &p);
			std::wstring name(p);
			CoTaskMemFree(p);

			PropVariantClear(&pv);

			bool found = false;
			for (int j = 0; j < sizeof(comp) / sizeof(std::wstring); j++) {
				if (endsWith(name, comp[j])) {
					found = true;
					break;
				}
			}

			if (!found) {
				continue;
			}
		}

		{
			IDeviceTopology* topology;
			hr = device->Activate(__uuidof(IDeviceTopology), CLSCTX_ALL, NULL, (void**)&topology);
			if (!SUCCEEDED(hr))
				continue;

			IConnector* connector;
			hr = topology->GetConnector(0, &connector);
			if (!SUCCEEDED(hr))
				continue;

			IConnector* connectedTo;
			hr = connector->GetConnectedTo(&connectedTo);
			if (!SUCCEEDED(hr))
				continue;

			IPart* part;
			hr = connectedTo->QueryInterface(&part);
			if (!SUCCEEDED(hr))
				continue;

			IKsJackDescription* jack;
			hr = part->Activate(CLSCTX_ALL, IID_PPV_ARGS(&jack));
			if (!SUCCEEDED(hr)) {
				continue;
			}

			UINT jackCount = 0;
			jack->GetJackCount(&jackCount);

			for (UINT j = 0; j < jackCount; j++) {
				KSJACK_DESCRIPTION desc = { 0 };
				jack->GetJackDescription(j, &desc);
				if (desc.ConnectionType == eConnType3Point5mm) {
					if (dataFlow == eRender) {
						headphones = device;
					}
					else if (dataFlow == eCapture) {
						mic = device;
					}
				}
				else if (desc.ConnectionType == eConnTypeAtapiInternal) {
					if (dataFlow == eRender) {
						speaker = device;
					}
				}
				else if (desc.ConnectionType == eConnTypeUnknown) {
					if (dataFlow == eCapture) {
						micarray = device;
					}
				}
			}
		}
	}
}