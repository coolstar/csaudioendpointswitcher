#include <Windows.h>
#include "AudioDeviceUtil.h"
#include <string>
#include <iostream>
extern "C" {
	#include "csaudioclient.h"
}

SERVICE_STATUS        g_ServiceStatus = { 0 };
SERVICE_STATUS_HANDLE g_StatusHandle = NULL;
HANDLE                g_ServiceStopEvent = INVALID_HANDLE_VALUE;

VOID WINAPI ServiceMain(DWORD argc, LPTSTR* argv);
VOID WINAPI ServiceCtrlHandler(DWORD);
DWORD WINAPI ServiceWorkerThread(LPVOID lpParam);

#define SERVICE_NAME  (LPWSTR)L"CoolStar Audio Service"

int WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR     lpCmdLine, int       nShowCmd)
{
	SERVICE_TABLE_ENTRY ServiceTable[] =
	{
		{SERVICE_NAME, (LPSERVICE_MAIN_FUNCTION)ServiceMain},
		{NULL, NULL}
	};

	if (StartServiceCtrlDispatcher(ServiceTable) == FALSE)
	{
		return GetLastError();
	}

	return 0;
}

VOID WINAPI ServiceMain(DWORD argc, LPTSTR* argv)
{
    DWORD Status = E_FAIL;
    HANDLE hThread = NULL;

    // Register our service control handler with the SCM
    g_StatusHandle = RegisterServiceCtrlHandler(SERVICE_NAME, ServiceCtrlHandler);

    if (g_StatusHandle == NULL)
    {
        goto EXIT;
    }

    // Tell the service controller we are starting
    ZeroMemory(&g_ServiceStatus, sizeof(g_ServiceStatus));
    g_ServiceStatus.dwServiceType = SERVICE_WIN32_OWN_PROCESS;
    g_ServiceStatus.dwControlsAccepted = 0;
    g_ServiceStatus.dwCurrentState = SERVICE_START_PENDING;
    g_ServiceStatus.dwWin32ExitCode = 0;
    g_ServiceStatus.dwServiceSpecificExitCode = 0;
    g_ServiceStatus.dwCheckPoint = 0;

    if (SetServiceStatus(g_StatusHandle, &g_ServiceStatus) == FALSE)
    {
        OutputDebugString(L"CoolStar Audio Service: ServiceMain: SetServiceStatus returned error");
    }

    /*
     * Perform tasks necessary to start the service here
     */
     // Create a service stop event to wait on later
    g_ServiceStopEvent = CreateEvent(NULL, TRUE, FALSE, NULL);
    if (g_ServiceStopEvent == NULL)
    {
        // Error creating event
        // Tell service controller we are stopped and exit
        g_ServiceStatus.dwControlsAccepted = 0;
        g_ServiceStatus.dwCurrentState = SERVICE_STOPPED;
        g_ServiceStatus.dwWin32ExitCode = GetLastError();
        g_ServiceStatus.dwCheckPoint = 1;

        if (SetServiceStatus(g_StatusHandle, &g_ServiceStatus) == FALSE)
        {
            OutputDebugString(L"CoolStar Audio Service: ServiceMain: SetServiceStatus returned error");
        }
        goto EXIT;
    }

    // Tell the service controller we are started
    g_ServiceStatus.dwControlsAccepted = SERVICE_ACCEPT_STOP;
    g_ServiceStatus.dwCurrentState = SERVICE_RUNNING;
    g_ServiceStatus.dwWin32ExitCode = 0;
    g_ServiceStatus.dwCheckPoint = 0;

    if (SetServiceStatus(g_StatusHandle, &g_ServiceStatus) == FALSE)
    {
        OutputDebugString(L"CoolStar Audio Service: ServiceMain: SetServiceStatus returned error");
    }

    // Start a thread that will perform the main task of the service
    hThread = CreateThread(NULL, 0, ServiceWorkerThread, NULL, 0, NULL);

    //Wait until stop event
    WaitForSingleObject(g_ServiceStopEvent, INFINITE);
    // Wait until our worker thread exits signaling that the service needs to stop
    WaitForSingleObject(hThread, 1000); //give the thread 1000 ms

    /*
     * Perform any cleanup tasks
     */

    CloseHandle(g_ServiceStopEvent);

    // Tell the service controller we are stopped
    g_ServiceStatus.dwControlsAccepted = 0;
    g_ServiceStatus.dwCurrentState = SERVICE_STOPPED;
    g_ServiceStatus.dwWin32ExitCode = 0;
    g_ServiceStatus.dwCheckPoint = 3;

    if (SetServiceStatus(g_StatusHandle, &g_ServiceStatus) == FALSE)
    {
        OutputDebugString(L"CoolStar Audio Service: ServiceMain: SetServiceStatus returned error");
    }

EXIT:
    return;
}

VOID WINAPI ServiceCtrlHandler(DWORD CtrlCode)
{
    switch (CtrlCode)
    {
    case SERVICE_CONTROL_STOP:

        if (g_ServiceStatus.dwCurrentState != SERVICE_RUNNING)
            break;

        /*
         * Perform tasks necessary to stop the service here
         */

        g_ServiceStatus.dwControlsAccepted = 0;
        g_ServiceStatus.dwCurrentState = SERVICE_STOP_PENDING;
        g_ServiceStatus.dwWin32ExitCode = 0;
        g_ServiceStatus.dwCheckPoint = 4;

        if (SetServiceStatus(g_StatusHandle, &g_ServiceStatus) == FALSE)
        {
            OutputDebugString(L"CoolStar Audio Service: ServiceCtrlHandler: SetServiceStatus returned error");
        }

        // This will signal the worker thread to start shutting down
        SetEvent(g_ServiceStopEvent);

        break;

    default:
        break;
    }
}

DWORD WINAPI ServiceWorkerThread(LPVOID lpParam)
{
    pcsaudio_client client = csaudio_alloc();
    if (!csaudio_connect(client)) {
        std::cout << "Failed to connect to codec" << std::endl;
        return -1;
    }

    AudioDeviceUtil util;
    //  Periodically check if the service has been requested to stop
    while (WaitForSingleObject(g_ServiceStopEvent, 0) != WAIT_OBJECT_0)
    {
        CsAudioSpecialKeyReport specialKeyReport;
        if (csaudio_read_message(client, &specialKeyReport)) {
            if (specialKeyReport.ControlCode == CONTROL_CODE_JACK_TYPE) {
                bool isHeadphone = (specialKeyReport.ControlValue & SND_JACK_HEADPHONE);
                bool isMic = (specialKeyReport.ControlValue & SND_JACK_MICROPHONE);

                if (util.defaultIsExpected(eRender, !isHeadphone, eConsole)) {
                    util.setExpectedDevice(eRender, isHeadphone, eConsole);
                }
                if (util.defaultIsExpected(eRender, !isHeadphone, eCommunications)) {
                    util.setExpectedDevice(eRender, isHeadphone, eCommunications);
                }

                if (util.defaultIsExpected(eCapture, !isMic, eConsole)) {
                    util.setExpectedDevice(eCapture, isMic, eConsole);
                }
                if (util.defaultIsExpected(eCapture, !isMic, eCommunications)) {
                    util.setExpectedDevice(eCapture, isMic, eCommunications);
                }
            }
        }
        Sleep(100);
    }

    csaudio_disconnect(client);
    csaudio_free(client);

    return ERROR_SUCCESS;
}