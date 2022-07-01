#if !defined(_CSAUDIO_CLIENT_H_)
#define _CSAUDIO_CLIENT_H_

#include <windows.h>
#include "csaudiocommon.h"

typedef struct _csaudio_client_t* pcsaudio_client;

USHORT getVendorID();

pcsaudio_client csaudio_alloc(void);

void csaudio_free(pcsaudio_client vmulti);

BOOL csaudio_connect(pcsaudio_client vmulti);

void csaudio_disconnect(pcsaudio_client vmulti);

BOOL csaudio_read_message(pcsaudio_client vmulti, CsAudioSpecialKeyReport* pReport);

#endif