#include <mmdeviceapi.h>

class AudioDeviceUtil {
private:
	IMMDeviceEnumerator* pEnum = NULL;

	IMMDevice* speaker = NULL;
	IMMDevice* headphones = NULL;
	IMMDevice* micarray = NULL;
	IMMDevice* mic = NULL;

public:
	AudioDeviceUtil();
	bool defaultIsExpected(EDataFlow dataFlow, bool jackState, ERole role);
	bool setExpectedDevice(EDataFlow dataFlow, bool jackState, ERole role);
private:
	IMMDevice* expectedDevice(EDataFlow dataFlow, bool jackState);
	void findCoolstarAudio();
	void handleDevices(EDataFlow dataFlow, IMMDeviceCollection* devices);
};