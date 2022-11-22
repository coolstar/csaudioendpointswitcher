#if !defined(_CSAUDIO_COMMON_H_)
#define _CSAUDIO_COMMON_H_

//
//These are the device attributes returned by vmulti in response
// to IOCTL_HID_GET_DEVICE_ATTRIBUTES.
//


#define RT5682_PID              0x10EC
#define RT5682_VID              0x5682
#define RT5682_VERSION          0x0001

#define RT5682_PID2              0x5682 //some versions of the rt5682 driver are bugged and have mismatched VID / PID
#define RT5682_VID2              0x10EC

#define NAU8825_PID              0x8825
#define NAU8825_VID              0x1050
#define NAU8825_VERSION          0x0001

#define DA7219_PID              0x7219
#define DA7219_VID              0x2DCF
#define DA7219_VERSION          0x0001

#define RT5663_PID              0x10EC
#define RT5663_VID              0x5663
#define RT5663_VERSION          0x0001

//
// These are the report ids
//

#define REPORTID_MEDIA	0x01
#define REPORTID_SPECKEYS		0x02

#define CONTROL_CODE_JACK_TYPE 0x1

enum snd_jack_types {
	SND_JACK_HEADPHONE = 0x0001,
	SND_JACK_MICROPHONE = 0x0002,
	SND_JACK_HEADSET = SND_JACK_HEADPHONE | SND_JACK_MICROPHONE,
};

#pragma pack(1)
typedef struct _CSAUDIO_SPECKEY_REPORT
{

	BYTE      ReportID;

	BYTE	  ControlCode;

	BYTE	  ControlValue;

} CsAudioSpecialKeyReport;

#endif
#pragma once
